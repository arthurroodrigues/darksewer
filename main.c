#include "main.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

#define MAPA_ARQUIVO "assets\\mapa.png"
#define PAREDES_ARQUIVO "assets\\mapa_paredes.png"
#define TELA_INICIAL_ARQUIVO "assets\\tela_de_inicio.png"
#define MAPA_LARGURA 768
#define MAPA_ALTURA 991
#define ZOOM_CAMERA 4.0f
#define JOGADOR_INICIAL_X 96.0f
#define JOGADOR_INICIAL_Y 816.0f
#define MEIA_LARGURA_HITBOX_PES 2.5f
#define ALTURA_HITBOX_PES 2.0f
#define DESLOCAMENTO_HITBOX_PES_Y 2.0f
#define LARGURA_JANELA 960
#define ALTURA_JANELA 720

static Color *pixelsMascaraParedes;
static int larguraMascaraParedes;
static int alturaMascaraParedes;

static bool CarregarMascaraParedes(int larguraMapaFonte, int alturaMapaFonte){
    Image imagem = LoadImage(PAREDES_ARQUIVO);

    if (imagem.data == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar a mascara: %s", PAREDES_ARQUIVO);
        return false;
    }

    if (imagem.width != larguraMapaFonte ||
        (imagem.height != alturaMapaFonte && imagem.height != alturaMapaFonte + 1)) {
        TraceLog(LOG_ERROR, "Tamanho da mascara invalido: %d x %d; esperado %d x %d (ou uma linha extra).", imagem.width, imagem.height, larguraMapaFonte, alturaMapaFonte);
        UnloadImage(imagem);
        return false;
    }

    Color *pixels = LoadImageColors(imagem);
    UnloadImage(imagem);
    if (pixels == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar os pixels da mascara de paredes.");
        return false;
    }

    for (int y = 0; y < alturaMapaFonte; y++) {
        for (int x = 0; x < larguraMapaFonte; x++) {
            const int indice = y * larguraMapaFonte + x;
            const Color pixel = pixels[indice];
            const unsigned int brilho =
                (299u * pixel.r + 587u * pixel.g + 114u * pixel.b) / 1000u;
            pixels[indice] = brilho >= 128u ? BLACK : WHITE;
        }
    }

    pixelsMascaraParedes = pixels;
    larguraMascaraParedes = larguraMapaFonte;
    alturaMascaraParedes = alturaMapaFonte;
    return true;
}

static void LiberarMascaraParedes(void){
    if (pixelsMascaraParedes == NULL) return;
    UnloadImageColors(pixelsMascaraParedes);
    pixelsMascaraParedes = NULL;
}

static bool PixelParede(int x, int y){
    if (pixelsMascaraParedes == NULL || x < 0 || x >= larguraMascaraParedes ||
        y < 0 || y >= alturaMascaraParedes) return true;

    return pixelsMascaraParedes[y * larguraMascaraParedes + x].r < 128;
}

static bool PodeOcupar(Vector2 posicaoJogador){
    const float esquerda = posicaoJogador.x - MEIA_LARGURA_HITBOX_PES;
    const float direita = posicaoJogador.x + MEIA_LARGURA_HITBOX_PES;
    const float topo =
        posicaoJogador.y - DESLOCAMENTO_HITBOX_PES_Y - ALTURA_HITBOX_PES;
    const float base = posicaoJogador.y - DESLOCAMENTO_HITBOX_PES_Y;

    if (esquerda < 0.0f || topo < 0.0f ||
        direita > MAPA_LARGURA || base > MAPA_ALTURA) return false;

    const float escalaMascaraX = (float)larguraMascaraParedes / (float)MAPA_LARGURA;
    const float escalaMascaraY = (float)alturaMascaraParedes / (float)MAPA_ALTURA;
    const int pixelXInicial = (int)floorf(esquerda * escalaMascaraX);
    const int pixelXFinal = (int)ceilf(direita * escalaMascaraX) - 1;
    const int pixelYInicial = (int)floorf(topo * escalaMascaraY);
    const int pixelYFinal = (int)ceilf(base * escalaMascaraY) - 1;

    for (int y = pixelYInicial; y <= pixelYFinal; y++) {
        const float pixelTopo = (float)y / escalaMascaraY;
        const float pixelBase = (float)(y + 1) / escalaMascaraY;
        if (pixelTopo >= base || pixelBase <= topo) continue;

        for (int x = pixelXInicial; x <= pixelXFinal; x++) {
            const float pixelEsquerda = (float)x / escalaMascaraX;
            const float pixelDireita = (float)(x + 1) / escalaMascaraX;
            if (pixelEsquerda >= direita || pixelDireita <= esquerda) continue;
            if (PixelParede(x, y)) return false;
        }
    }
    return true;
}

static Vector2 MoverComColisao(Vector2 posicaoJogador, Vector2 deslocamentoJogador){
    const float maiorDistanciaMovida = fmaxf(fabsf(deslocamentoJogador.x), fabsf(deslocamentoJogador.y));
    const float passoMaximo = 0.5f * fminf(
        (float)MAPA_LARGURA / (float)larguraMascaraParedes,
        (float)MAPA_ALTURA / (float)alturaMascaraParedes);
    const int quantidadePassos = (int)ceilf(maiorDistanciaMovida / passoMaximo);
    if (quantidadePassos <= 0) return posicaoJogador;

    const Vector2 deslocamentoPorPasso = Vector2Scale(deslocamentoJogador, 1.0f / (float)quantidadePassos);
    for (int i = 0; i < quantidadePassos; i++) {
        Vector2 posicaoTentativaX = {
            posicaoJogador.x + deslocamentoPorPasso.x, posicaoJogador.y
        };

        if (PodeOcupar(posicaoTentativaX)) posicaoJogador.x = posicaoTentativaX.x;

        Vector2 posicaoTentativaY = {
            posicaoJogador.x, posicaoJogador.y + deslocamentoPorPasso.y
        };
        if (PodeOcupar(posicaoTentativaY)) posicaoJogador.y = posicaoTentativaY.y;
    }
    return posicaoJogador;
}

static Rectangle CalcularDestinoTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela){
    const float escala = fmaxf((float)larguraTela / (float)imagemFundo.width, (float)alturaTela / (float)imagemFundo.height);
    const float larguraImagem = (float)imagemFundo.width * escala;
    const float alturaImagem = (float)imagemFundo.height * escala;

    return (Rectangle){
        ((float)larguraTela - larguraImagem) * 0.5f,
        ((float)alturaTela - alturaImagem) * 0.5f,
        larguraImagem, alturaImagem
    };
}

static Rectangle CalcularBotaoTelaInicial(Texture2D imagemFundo, Rectangle destinoImagemFundo){
    const Rectangle areaBotaoNaImagem = {
        (float)imagemFundo.width * 0.416f,
        (float)imagemFundo.height * 0.733f,
        (float)imagemFundo.width * 0.168f,
        (float)imagemFundo.height * 0.077f
    };

    const float escalaX = destinoImagemFundo.width / (float)imagemFundo.width;
    const float escalaY = destinoImagemFundo.height / (float)imagemFundo.height;
    return (Rectangle){
        destinoImagemFundo.x + areaBotaoNaImagem.x * escalaX,
        destinoImagemFundo.y + areaBotaoNaImagem.y * escalaY,
        areaBotaoNaImagem.width * escalaX,
        areaBotaoNaImagem.height * escalaY
    };
}

static void DesenharTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela, float opacidade){
    ClearBackground((Color){1, 3, 4, 255});
    const Rectangle origemImagemFundo = {
        0.0f, 0.0f, (float)imagemFundo.width, (float)imagemFundo.height
    };
    const Rectangle destinoImagemFundo =
        CalcularDestinoTelaInicial(imagemFundo, larguraTela, alturaTela);
    DrawTexturePro(imagemFundo, origemImagemFundo, destinoImagemFundo,
                   (Vector2){0.0f, 0.0f}, 0.0f,
                   (Color){255, 255, 255, (unsigned char)(255.0f * opacidade)});
}

int main(void){
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Dark Sewer");
    SetWindowMinSize(480, 360);
    SetTargetFPS(60);

    if (!FileExists(MAPA_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Nao encontrei mapa.png. Coloque-o ao lado de main.c.");
        CloseWindow();
        return 1;
    }

    Texture2D mapa = LoadTexture(MAPA_ARQUIVO);

    if (mapa.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar mapa.png.");
        CloseWindow();
        return 1;
    }

    if (mapa.width <= 0 || mapa.height <= 0) {
        TraceLog(LOG_ERROR, "mapa.png tem dimensoes invalidas (%d x %d).", mapa.width, mapa.height);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    if (!FileExists(TELA_INICIAL_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Nao encontrei tela_de_inicio.png.");
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    Texture2D telaInicial = LoadTexture(TELA_INICIAL_ARQUIVO);

    if (telaInicial.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar tela_de_inicio.png.");
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    if (!CarregarMascaraParedes(mapa.width, mapa.height)) {
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    Vector2 jogador = {JOGADOR_INICIAL_X, JOGADOR_INICIAL_Y};

    if (!PodeOcupar(jogador)) {
        TraceLog(LOG_ERROR, "A posicao inicial do jogador esta bloqueada pela mascara.");
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    Camera2D camera = {0};
    camera.offset = (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera.zoom = ZOOM_CAMERA;

    bool jogoIniciado = false;
    float opacidadeTelaInicial = 0.0f;

    while (!WindowShouldClose()) {
        const float deltaTempo = fminf(GetFrameTime(), 0.05f);
        const int larguraTela = GetScreenWidth();
        const int alturaTela = GetScreenHeight();

        if (!jogoIniciado) {
            opacidadeTelaInicial = fminf(opacidadeTelaInicial + deltaTempo * 0.5f, 1.0f);
            const Rectangle destinoTelaInicial = CalcularDestinoTelaInicial(telaInicial, larguraTela, alturaTela);
            const Rectangle botaoJogar = CalcularBotaoTelaInicial(telaInicial, destinoTelaInicial);
            const bool mouseSobreBotao = CheckCollisionPointRec(GetMousePosition(), botaoJogar);
            SetMouseCursor(mouseSobreBotao ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                (mouseSobreBotao && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                jogoIniciado = true;
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            EndDrawing();
            continue;
        }

        camera.offset = (Vector2){larguraTela * 0.5f, alturaTela * 0.5f};
        camera.zoom = ZOOM_CAMERA;
        const float metadeLarguraVisivel = larguraTela / (2.0f * camera.zoom);
        const float metadeAlturaVisivel = alturaTela / (2.0f * camera.zoom);
        camera.target.x = fmaxf(metadeLarguraVisivel, fminf(jogador.x, MAPA_LARGURA - metadeLarguraVisivel));
        camera.target.y = fmaxf(metadeAlturaVisivel, fminf(jogador.y, MAPA_ALTURA - metadeAlturaVisivel));

        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        BeginMode2D(camera);
        const Rectangle origemMapa = {0.0f, 0.0f, (float)mapa.width, (float)mapa.height};
        const Rectangle destinoMapa = {0.0f, 0.0f, (float)MAPA_LARGURA, (float)MAPA_ALTURA};
        DrawTexturePro(mapa, origemMapa, destinoMapa, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndMode2D();
        EndDrawing();
    }

    LiberarMascaraParedes();
    UnloadTexture(telaInicial);
    UnloadTexture(mapa);
    CloseWindow();

    return 0;
}