#include "main.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAPA_ARQUIVO "assets\\mapa.png"
#define PAREDES_ARQUIVO "assets\\mapa_paredes.png"
#define TELA_INICIAL_ARQUIVO "assets\\tela_de_inicio.png"
#define MAPA_LARGURA 768
#define MAPA_ALTURA 991
#define INTERVALO_ANIMACAO 0.18f
#define ZOOM_CAMERA 4.0f
#define RAIO_LANTERNA 100.0f
#define INTENSIDADE_ESCURO 1.0f
#define VELOCIDADE_JOGADOR 112.0f
#define JOGADOR_INICIAL_X 96.0f
#define JOGADOR_INICIAL_Y 816.0f
#define MEIA_LARGURA_HITBOX_PES 2.5f
#define ALTURA_HITBOX_PES 2.0f
#define DESLOCAMENTO_HITBOX_PES_Y 2.0f
#define ALTURA_SPRITE_JOGADOR 32.5f
#define ARQUIVO_SPRITE_PARADO "assets\\jogador_parado.png"
#define ARQUIVO_SPRITE_ANDANDO "assets\\jogador_andando.png"
#define ARQUIVO_SPRITE_PARADO_ESQUERDA "assets\\jogador_parado_esquerda.png"
#define ARQUIVO_SPRITE_ANDANDO_ESQUERDA "assets\\jogador_andando_esquerda.png"
#define ARQUIVO_SPRITE_FRENTE "assets\\jogador_andando_para_frente.png"
#define ARQUIVO_SPRITE_PARADO_FRENTE "assets\\jogador_parado_para_frente.png"
#define ARQUIVO_SPRITE_TRAS "assets\\jogador_andando_para_tras.png"
#define ARQUIVO_SPRITE_PARADO_TRAS "assets\\jogador_parado_para_tras.png"
#define LARGURA_JANELA 960
#define ALTURA_JANELA 720

static Color *pixelsMascaraParedes;
static int larguraMascaraParedes;
static int alturaMascaraParedes;

static const char *kShaderLanterna =
    "#version 330\n"
    "uniform vec2 uLightPosition;\n"
    "uniform float uRadius;\n"
    "uniform float uDarkness;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "vec2 delta = gl_FragCoord.xy - uLightPosition;\n"
    "float distanceToLight = length(delta);\n"
    "float radialEdge = smoothstep(uRadius * 0.48, uRadius, distanceToLight);\n"
    "float alpha = mix(0.13, uDarkness, radialEdge);\n"
    "vec3 color = mix(vec3(0.17, 0.12, 0.045), vec3(0.003, 0.010, 0.008), radialEdge);\n"
    "finalColor = vec4(color, alpha);\n"
    "}\n";

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

static void DesenharJogador(Vector2 posicaoJogador, Texture2D spriteJogador){
    const float escala = ALTURA_SPRITE_JOGADOR / (float)spriteJogador.height;
    const float largura = (float)spriteJogador.width * escala;
    const float altura = (float)spriteJogador.height * escala;
    const Rectangle origem = {
        0.0f, 0.0f, (float)spriteJogador.width, (float)spriteJogador.height
    };
    const Rectangle destino = {posicaoJogador.x, posicaoJogador.y, largura, altura};
    const Vector2 ancora = {largura * 0.5f, altura};
    DrawTexturePro(spriteJogador, origem, destino, ancora, 0.0f, WHITE);
}

static bool ResolverArquivoAudio(const char *caminhoRelativo, char *bufferCaminho,
                                 size_t tamanhoBuffer){
    const char *diretorioAplicacao = GetApplicationDirectory();
    int caracteresEscritos =
        snprintf(bufferCaminho, tamanhoBuffer, "%s%s", diretorioAplicacao,
                 caminhoRelativo);
    if (caracteresEscritos >= 0 && (size_t)caracteresEscritos < tamanhoBuffer &&
        FileExists(bufferCaminho)) {
        return true;
    }

    caracteresEscritos = snprintf(bufferCaminho, tamanhoBuffer, "%s", caminhoRelativo);
    if (caracteresEscritos >= 0 && (size_t)caracteresEscritos < tamanhoBuffer &&
        FileExists(bufferCaminho)) {
        return true;
    }

    bufferCaminho[0] = '\0';
    TraceLog(LOG_ERROR, "Arquivo de audio nao encontrado: %s", caminhoRelativo);
    return false;
}

static bool InicializarSons(Sons *estadoSons){
    char caminhoMusica[512];
    char caminhoPassos[512];

    if (!ResolverArquivoAudio("sons\\musica_fundo.mp3", caminhoMusica,sizeof(caminhoMusica)) ||
        !ResolverArquivoAudio("sons\\passos.mp3", caminhoPassos, sizeof(caminhoPassos))) {
        TraceLog(LOG_WARNING, "Audio incompleto; a gameplay continuara sem audio.");
        return false;
    }

    InitAudioDevice();

    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_WARNING, "Audio indisponivel; a gameplay continuara sem audio.");
        return false;
    }

    estadoSons->dispositivoAudioAtivo = true;
    estadoSons->musicaFundo = LoadMusicStream(caminhoMusica);
    estadoSons->musicaPassos = LoadMusicStream(caminhoPassos);

    if (!IsMusicValid(estadoSons->musicaFundo) ||
        !IsMusicValid(estadoSons->musicaPassos)) {
        TraceLog(LOG_WARNING, "Falha ao carregar um ou mais audios; audio desativado.");
        return false;
    }

    estadoSons->musicaFundo.looping = true;
    estadoSons->musicaPassos.looping = true;
    return true;
}

static void LiberarSons(Sons *estadoSons){
    if (!estadoSons->dispositivoAudioAtivo) return;

    if (IsMusicValid(estadoSons->musicaFundo)) {
        StopMusicStream(estadoSons->musicaFundo);
        UnloadMusicStream(estadoSons->musicaFundo);
    }

    if (IsMusicValid(estadoSons->musicaPassos)) {
        StopMusicStream(estadoSons->musicaPassos);
        UnloadMusicStream(estadoSons->musicaPassos);
    }

    CloseAudioDevice();
    estadoSons->dispositivoAudioAtivo = false;
}

static void AtualizarPassos(Sons *estadoSons, bool teclasMovimentoPressionadas){
    if (!estadoSons->dispositivoAudioAtivo) return;

    if (teclasMovimentoPressionadas) {
        if (!estadoSons->somPassosTocando) {
            PlayMusicStream(estadoSons->musicaPassos);
            estadoSons->somPassosTocando = true;
        }

        UpdateMusicStream(estadoSons->musicaPassos);

    } else if (estadoSons->somPassosTocando) {
        StopMusicStream(estadoSons->musicaPassos);
        estadoSons->somPassosTocando = false;
    }
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

typedef struct loot {
    int numero;
    struct loot *anterior;
    struct loot *proximo;
} loot;

static loot *CriarHotbar(void){
    loot *primeiro = NULL;
    loot *ultimo = NULL;

    for (int i = 0; i < 5; i++) {
        loot *novo = malloc(sizeof(loot));
        novo->numero = i + 1;
        novo->anterior = ultimo;
        novo->proximo = NULL;

        if (ultimo == NULL) primeiro = novo;
        else ultimo->proximo = novo;
        ultimo = novo;
    }

    ultimo->proximo = primeiro;
    primeiro->anterior = ultimo;
    return primeiro;
}

static void LiberarHotbar(loot *primeiro){
    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        loot *proximo = no->proximo;
        free(no);
        no = proximo;
    }
}

static void DesenharHotbar(loot *primeiro, loot *selecionado, int larguraTela, int alturaTela){
    const int tamanho = 48;
    const int espaco = 6;
    const int inicioX = (larguraTela - (5 * tamanho + 4 * espaco)) / 2;
    const int y = alturaTela - tamanho - 20;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        const int x = inicioX + i * (tamanho + espaco);
        const bool ativo = (no == selecionado);

        DrawRectangle(x, y, tamanho, tamanho, (Color){10, 16, 15, 200});
        DrawRectangleLinesEx((Rectangle){x, y, tamanho, tamanho}, ativo ? 3 : 2, ativo ? (Color){176, 18, 24, 255} : (Color){74, 88, 82, 255});
        no = no->proximo;
    }
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

    if (!FileExists(ARQUIVO_SPRITE_PARADO) || !FileExists(ARQUIVO_SPRITE_ANDANDO) ||
        !FileExists(ARQUIVO_SPRITE_PARADO_ESQUERDA) ||
        !FileExists(ARQUIVO_SPRITE_ANDANDO_ESQUERDA) ||
        !FileExists(ARQUIVO_SPRITE_FRENTE) || !FileExists(ARQUIVO_SPRITE_PARADO_FRENTE) ||
        !FileExists(ARQUIVO_SPRITE_TRAS) || !FileExists(ARQUIVO_SPRITE_PARADO_TRAS)) {
        TraceLog(LOG_ERROR, "Sprites ausentes. Verifique os arquivos dentro da pasta assets.");
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    Texture2D spriteParado = LoadTexture(ARQUIVO_SPRITE_PARADO);
    Texture2D spriteAndando = LoadTexture(ARQUIVO_SPRITE_ANDANDO);
    Texture2D spriteParadoEsquerda = LoadTexture(ARQUIVO_SPRITE_PARADO_ESQUERDA);
    Texture2D spriteAndandoEsquerda = LoadTexture(ARQUIVO_SPRITE_ANDANDO_ESQUERDA);
    Texture2D spriteFrente = LoadTexture(ARQUIVO_SPRITE_FRENTE);
    Texture2D spriteParadoFrente = LoadTexture(ARQUIVO_SPRITE_PARADO_FRENTE);
    Texture2D spriteTras = LoadTexture(ARQUIVO_SPRITE_TRAS);
    Texture2D spriteParadoTras = LoadTexture(ARQUIVO_SPRITE_PARADO_TRAS);

    if (spriteParado.id == 0 || spriteAndando.id == 0 ||
        spriteParadoEsquerda.id == 0 || spriteAndandoEsquerda.id == 0 ||
        spriteFrente.id == 0 || spriteParadoFrente.id == 0 ||
        spriteTras.id == 0 || spriteParadoTras.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar os sprites do jogador.");
        if (spriteParado.id != 0) UnloadTexture(spriteParado);
        if (spriteAndando.id != 0) UnloadTexture(spriteAndando);
        if (spriteParadoEsquerda.id != 0) UnloadTexture(spriteParadoEsquerda);
        if (spriteAndandoEsquerda.id != 0) UnloadTexture(spriteAndandoEsquerda);
        if (spriteFrente.id != 0) UnloadTexture(spriteFrente);
        if (spriteParadoFrente.id != 0) UnloadTexture(spriteParadoFrente);
        if (spriteTras.id != 0) UnloadTexture(spriteTras);
        if (spriteParadoTras.id != 0) UnloadTexture(spriteParadoTras);
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    SetTextureFilter(spriteParado, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteAndando, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteParadoEsquerda, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteAndandoEsquerda, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteFrente, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteParadoFrente, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteTras, TEXTURE_FILTER_POINT);
    SetTextureFilter(spriteParadoTras, TEXTURE_FILTER_POINT);

    Sons sonsJogo = {0};
    
    const bool audioDisponivel = InicializarSons(&sonsJogo);
    if (!audioDisponivel) {
        LiberarSons(&sonsJogo);
    } else {
        PlayMusicStream(sonsJogo.musicaFundo);
    }

    Shader shaderLanterna = LoadShaderFromMemory(NULL, kShaderLanterna);
    if (!IsShaderValid(shaderLanterna)) {
        TraceLog(LOG_ERROR, "Nao foi possivel criar o shader da lanterna.");
        LiberarSons(&sonsJogo);
        UnloadTexture(spriteParado);
        UnloadTexture(spriteAndando);
        UnloadTexture(spriteParadoEsquerda);
        UnloadTexture(spriteAndandoEsquerda);
        UnloadTexture(spriteFrente);
        UnloadTexture(spriteParadoFrente);
        UnloadTexture(spriteTras);
        UnloadTexture(spriteParadoTras);
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    const int localLuzShader = GetShaderLocation(shaderLanterna, "uLightPosition");
    const int localRaioShader = GetShaderLocation(shaderLanterna, "uRadius");
    const int localEscuridaoShader = GetShaderLocation(shaderLanterna, "uDarkness");

    float tempoAnimacaoAndando = 0.0f;
    bool olhandoEsquerda = false;
    bool olhandoParaFrente = false;
    bool olhandoVertical = false;
    bool quadroAnimacaoAndando = false;

    Camera2D camera = {0};
    camera.offset = (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera.zoom = ZOOM_CAMERA;

    bool jogoIniciado = false;
    float opacidadeTelaInicial = 0.0f;

    loot *primeiroSlot = CriarHotbar();
    loot *slotSelecionado = primeiroSlot;

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

            if (audioDisponivel) UpdateMusicStream(sonsJogo.musicaFundo);

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_E) || GetMouseWheelMove() < 0.0f) slotSelecionado = slotSelecionado->proximo;
        if (IsKeyPressed(KEY_Q) || GetMouseWheelMove() > 0.0f) slotSelecionado = slotSelecionado->anterior;

        Vector2 direcaoMovimento = {0.0f, 0.0f};

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) direcaoMovimento.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direcaoMovimento.x += 1.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) direcaoMovimento.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direcaoMovimento.y += 1.0f;
        const bool teclasMovimentoPressionadas = Vector2LengthSqr(direcaoMovimento) > 0.0f;
        const Vector2 posicaoAnteriorJogador = jogador;

        if (teclasMovimentoPressionadas) {
            direcaoMovimento = Vector2Normalize(direcaoMovimento);
            jogador = MoverComColisao(jogador, Vector2Scale(direcaoMovimento, VELOCIDADE_JOGADOR * deltaTempo));
        }

        const bool jogadorEstaSeMovendo = Vector2DistanceSqr(jogador, posicaoAnteriorJogador) > 0.0001f;
        const float deslocamentoJogadorX = jogador.x - posicaoAnteriorJogador.x;
        const float deslocamentoJogadorY = jogador.y - posicaoAnteriorJogador.y;
        const bool movimentoVertical = jogadorEstaSeMovendo && fabsf(deslocamentoJogadorY) > fabsf(deslocamentoJogadorX);

        if (jogadorEstaSeMovendo) {
            if (movimentoVertical) {
                olhandoParaFrente = deslocamentoJogadorY > 0.0f;
                olhandoVertical = true;
            } else {
                olhandoEsquerda = deslocamentoJogadorX < 0.0f;
                olhandoVertical = false;
            }

            tempoAnimacaoAndando += deltaTempo;
            if (tempoAnimacaoAndando >= INTERVALO_ANIMACAO) {
                tempoAnimacaoAndando -= INTERVALO_ANIMACAO;
                quadroAnimacaoAndando = !quadroAnimacaoAndando;
            }
        } else {
            tempoAnimacaoAndando = 0.0f;
            quadroAnimacaoAndando = false;
        }

        camera.offset = (Vector2){larguraTela * 0.5f, alturaTela * 0.5f};
        camera.zoom = ZOOM_CAMERA;
        const float metadeLarguraVisivel = larguraTela / (2.0f * camera.zoom);
        const float metadeAlturaVisivel = alturaTela / (2.0f * camera.zoom);
        camera.target.x = fmaxf(metadeLarguraVisivel, fminf(jogador.x, MAPA_LARGURA - metadeLarguraVisivel));
        camera.target.y = fmaxf(metadeAlturaVisivel, fminf(jogador.y, MAPA_ALTURA - metadeAlturaVisivel));
        
        if (sonsJogo.dispositivoAudioAtivo) {
            UpdateMusicStream(sonsJogo.musicaFundo);
            AtualizarPassos(&sonsJogo, teclasMovimentoPressionadas);
        }

        const Vector2 posicaoJogadorNaTela = GetWorldToScreen2D(jogador, camera);
        const Vector2 posicaoLuzShader = {
            posicaoJogadorNaTela.x,
            (float)alturaTela - posicaoJogadorNaTela.y
        };

        const float raioLuzShader = RAIO_LANTERNA * camera.zoom;

        SetShaderValue(shaderLanterna, localLuzShader, &posicaoLuzShader, SHADER_UNIFORM_VEC2);
        SetShaderValue(shaderLanterna, localRaioShader, &raioLuzShader, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shaderLanterna, localEscuridaoShader, &(float){INTENSIDADE_ESCURO}, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        BeginMode2D(camera);
        const Rectangle origemMapa = {0.0f, 0.0f, (float)mapa.width, (float)mapa.height};
        const Rectangle destinoMapa = {0.0f, 0.0f, (float)MAPA_LARGURA, (float)MAPA_ALTURA};
        DrawTexturePro(mapa, origemMapa, destinoMapa, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        const Texture2D spriteAtual = olhandoVertical
            ? (olhandoParaFrente
                ? (quadroAnimacaoAndando ? spriteFrente : spriteParadoFrente)
                : (quadroAnimacaoAndando ? spriteTras : spriteParadoTras))
            : (olhandoEsquerda
                ? (jogadorEstaSeMovendo && quadroAnimacaoAndando
                    ? spriteAndandoEsquerda : spriteParadoEsquerda)
                : (jogadorEstaSeMovendo && quadroAnimacaoAndando
                    ? spriteAndando : spriteParado));
        DesenharJogador(jogador, spriteAtual);

        EndMode2D();

        BeginShaderMode(shaderLanterna);
            DrawRectangle(0, 0, larguraTela, alturaTela, WHITE);
        EndShaderMode();

        DesenharHotbar(primeiroSlot, slotSelecionado, larguraTela, alturaTela);

        EndDrawing();
    }

    LiberarHotbar(primeiroSlot);
    UnloadShader(shaderLanterna);
    LiberarSons(&sonsJogo);
    UnloadTexture(spriteParado);
    UnloadTexture(spriteAndando);
    UnloadTexture(spriteParadoEsquerda);
    UnloadTexture(spriteAndandoEsquerda);
    UnloadTexture(spriteFrente);
    UnloadTexture(spriteParadoFrente);
    UnloadTexture(spriteTras);
    UnloadTexture(spriteParadoTras);
    LiberarMascaraParedes();
    UnloadTexture(telaInicial);
    UnloadTexture(mapa);
    CloseWindow();

    return 0;
}