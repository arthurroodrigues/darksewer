#include "main.h"
#include "som.h"
#include "colisao.h"
#include "barra_itens.h"
#include "menu.h"
#include "jogador.h"
#include "lanterna.h"
#include "raymath.h"
#include <math.h>
#include <time.h>

#define MAPA_ARQUIVO "assets\\mapa.png"
#define TELA_INICIAL_ARQUIVO "assets\\tela_de_inicio.png"
#define BATERIA_ARQUIVO "assets\\bateria.png"
#define INTERVALO_ANIMACAO 0.18f
#define ZOOM_CAMERA 7.5f
#define INTENSIDADE_ESCURO 0.99f
#define TAMANHO_BATERIA_MAPA 15.0f
#define RAIO_COLETA_BATERIA 12.5f
#define DISTANCIA_MINIMA_SPAWN_BATERIA 24.0f
#define DISTANCIA_MINIMA_ENTRE_BATERIAS 32.0f
#define QUANTIDADE_BATERIAS 3
#define VELOCIDADE_JOGADOR 112.0f
#define JOGADOR_INICIAL_X 100.0f
#define JOGADOR_INICIAL_Y 600.0f
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

typedef enum {
    ESTADO_MENU,
    ESTADO_JOGANDO,
    ESTADO_PAUSADO
} EstadoJogo;

typedef struct {
    Vector2 jogador;
    Vector2 posicoesBateria[QUANTIDADE_BATERIAS];
    bool bateriasColetadas[QUANTIDADE_BATERIAS];
    Lanterna lanterna;
    loot *slotSelecionado;
    float tempoAnimacaoAndando;
    bool olhandoEsquerda;
    bool olhandoParaFrente;
    bool olhandoVertical;
    bool quadroAnimacaoAndando;
} Partida;

static bool IniciarPartida(Partida *partida, loot *primeiroSlot){
    *partida = (Partida){0};
    partida->jogador = (Vector2){JOGADOR_INICIAL_X, JOGADOR_INICIAL_Y};
    partida->lanterna = CriarLanterna();
    partida->slotSelecionado = primeiroSlot;
    EsvaziarHotbar(primeiroSlot);

    for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
        if (!EncontrarPosicaoAcessivelAleatoria(
                partida->jogador, DISTANCIA_MINIMA_SPAWN_BATERIA,
                partida->posicoesBateria, i, DISTANCIA_MINIMA_ENTRE_BATERIAS,
                &partida->posicoesBateria[i])) return false;
    }
    return true;
}

static void AtualizarPartida(Partida *partida, loot *primeiroSlot, Sons *sons, float deltaTempo){
    if (IsKeyPressed(KEY_E) || GetMouseWheelMove() < 0.0f) partida->slotSelecionado = partida->slotSelecionado->proximo;
    if (IsKeyPressed(KEY_Q) || GetMouseWheelMove() > 0.0f) partida->slotSelecionado = partida->slotSelecionado->anterior;

    if (IsKeyPressed(KEY_F)) AlternarLanterna(&partida->lanterna);
    if ((IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) &&
        partida->slotSelecionado->item == ITEM_BATERIA && RecarregarLanterna(&partida->lanterna)) {
        RemoverItemHotbar(partida->slotSelecionado);
    }
    AtualizarLanterna(&partida->lanterna, deltaTempo);

    Vector2 direcaoMovimento = {0.0f, 0.0f};

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) direcaoMovimento.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direcaoMovimento.x += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) direcaoMovimento.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direcaoMovimento.y += 1.0f;

    const bool teclasMovimentoPressionadas = Vector2LengthSqr(direcaoMovimento) > 0.0f;
    const Vector2 posicaoAnteriorJogador = partida->jogador;

    if (teclasMovimentoPressionadas) {
        direcaoMovimento = Vector2Normalize(direcaoMovimento);
        partida->jogador = MoverComColisao(partida->jogador, Vector2Scale(direcaoMovimento, VELOCIDADE_JOGADOR * deltaTempo));
    }

    const bool jogadorEstaSeMovendo = Vector2DistanceSqr(partida->jogador, posicaoAnteriorJogador) > 0.0001f;

    for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
        if (!partida->bateriasColetadas[i] &&
            Vector2DistanceSqr(partida->jogador, partida->posicoesBateria[i]) <=
                RAIO_COLETA_BATERIA * RAIO_COLETA_BATERIA) {
            partida->bateriasColetadas[i] =
                AdicionarItemHotbar(primeiroSlot, ITEM_BATERIA);
            if (partida->bateriasColetadas[i]) TocarSomColeta(sons);
        }
    }

    if (jogadorEstaSeMovendo) {
        const float intensidadeHorizontal = fabsf(direcaoMovimento.x);
        const float intensidadeVertical = fabsf(direcaoMovimento.y);

        if (intensidadeVertical > intensidadeHorizontal ||
            (intensidadeVertical == intensidadeHorizontal && partida->olhandoVertical)) {
            partida->olhandoParaFrente = direcaoMovimento.y > 0.0f;
            partida->olhandoVertical = true;
        } else {
            partida->olhandoEsquerda = direcaoMovimento.x < 0.0f;
            partida->olhandoVertical = false;
        }

        partida->tempoAnimacaoAndando += deltaTempo;

        if (partida->tempoAnimacaoAndando >= INTERVALO_ANIMACAO) {
            partida->tempoAnimacaoAndando -= INTERVALO_ANIMACAO;
            partida->quadroAnimacaoAndando = !partida->quadroAnimacaoAndando;
        }
    } else {
        partida->tempoAnimacaoAndando = 0.0f;
        partida->quadroAnimacaoAndando = false;
    }

    AtualizarPassos(sons, teclasMovimentoPressionadas);
}

int main(void){
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Dark Sewer");
    SetWindowMinSize(480, 360);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    if (!FileExists(MAPA_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Nao encontrei mapa.png dentro da pasta assets.");
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

    if (!CarregarMascaraParedes()) {
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    if (!PodeOcupar((Vector2){JOGADOR_INICIAL_X, JOGADOR_INICIAL_Y})) {
        TraceLog(LOG_ERROR, "A posicao inicial do jogador esta bloqueada pela mascara.");
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    SetRandomSeed((unsigned int)time(NULL) ^ (unsigned int)(GetTime() * 1000000.0));

    if (!FileExists(ARQUIVO_SPRITE_PARADO) || !FileExists(ARQUIVO_SPRITE_ANDANDO) ||
        !FileExists(ARQUIVO_SPRITE_PARADO_ESQUERDA) ||
        !FileExists(ARQUIVO_SPRITE_ANDANDO_ESQUERDA) ||
        !FileExists(ARQUIVO_SPRITE_FRENTE) || !FileExists(ARQUIVO_SPRITE_PARADO_FRENTE) ||
        !FileExists(ARQUIVO_SPRITE_TRAS) || !FileExists(ARQUIVO_SPRITE_PARADO_TRAS) ||
        !FileExists(BATERIA_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Sprites ou bateria ausentes. Verifique os arquivos dentro da pasta assets.");
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
    Texture2D bateria = LoadTexture(BATERIA_ARQUIVO);

    if (spriteParado.id == 0 || spriteAndando.id == 0 ||
        spriteParadoEsquerda.id == 0 || spriteAndandoEsquerda.id == 0 ||
        spriteFrente.id == 0 || spriteParadoFrente.id == 0 ||
        spriteTras.id == 0 || spriteParadoTras.id == 0 || bateria.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar os sprites do jogador ou a bateria.");
        if (spriteParado.id != 0) UnloadTexture(spriteParado);
        if (spriteAndando.id != 0) UnloadTexture(spriteAndando);
        if (spriteParadoEsquerda.id != 0) UnloadTexture(spriteParadoEsquerda);
        if (spriteAndandoEsquerda.id != 0) UnloadTexture(spriteAndandoEsquerda);
        if (spriteFrente.id != 0) UnloadTexture(spriteFrente);
        if (spriteParadoFrente.id != 0) UnloadTexture(spriteParadoFrente);
        if (spriteTras.id != 0) UnloadTexture(spriteTras);
        if (spriteParadoTras.id != 0) UnloadTexture(spriteParadoTras);
        if (bateria.id != 0) UnloadTexture(bateria);
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
    SetTextureFilter(bateria, TEXTURE_FILTER_BILINEAR);

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
        UnloadTexture(bateria);
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    const int localLuzShader = GetShaderLocation(shaderLanterna, "uLightPosition");
    const int localRaioShader = GetShaderLocation(shaderLanterna, "uRadius");
    const int localEscuridaoShader = GetShaderLocation(shaderLanterna, "uDarkness");

    Camera2D camera = {0};
    camera.offset = (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera.zoom = ZOOM_CAMERA;

    EstadoJogo estado = ESTADO_MENU;
    float opacidadeTelaInicial = 0.0f;

    loot *primeiroSlot = CriarHotbar();
    if (primeiroSlot == NULL) {
        TraceLog(LOG_ERROR, "Falha ao criar a barra de itens.");
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
        UnloadTexture(bateria);
        LiberarMascaraParedes();
        UnloadTexture(telaInicial);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }
    Partida partida = {0};
    int codigoSaida = 0;

    while (!WindowShouldClose()) {
        const float deltaTempo = fminf(GetFrameTime(), 0.05f);
        const int larguraTela = GetScreenWidth();
        const int alturaTela = GetScreenHeight();

        if (estado == ESTADO_MENU) {
            opacidadeTelaInicial = fminf(opacidadeTelaInicial + deltaTempo * 0.5f, 1.0f);
            const Rectangle destinoTelaInicial = CalcularDestinoTelaInicial(telaInicial, larguraTela, alturaTela);
            const Rectangle botaoJogar = CalcularBotaoTelaInicial(telaInicial, destinoTelaInicial);
            const bool mouseSobreBotao = CheckCollisionPointRec(GetMousePosition(), botaoJogar);
            SetMouseCursor(mouseSobreBotao ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
                (mouseSobreBotao && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                if (!IniciarPartida(&partida, primeiroSlot)) {
                    codigoSaida = 1;
                    break;
                }
                estado = ESTADO_JOGANDO;
                if (audioDisponivel) StopMusicStream(sonsJogo.musicaFundo);
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (audioDisponivel && estado == ESTADO_MENU) UpdateMusicStream(sonsJogo.musicaFundo);

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            EndDrawing();
            continue;
        }

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            estado = (estado == ESTADO_JOGANDO) ? ESTADO_PAUSADO : ESTADO_JOGANDO;
        }

        if (estado == ESTADO_PAUSADO) {
            AtualizarPassos(&sonsJogo, false);
            if (IsKeyPressed(KEY_Q)) break;
            if (IsKeyPressed(KEY_M)) {
                estado = ESTADO_MENU;
                if (audioDisponivel) PlayMusicStream(sonsJogo.musicaFundo);
                continue;
            }
        } else {
            AtualizarPartida(&partida, primeiroSlot, &sonsJogo, deltaTempo);
        }

        camera.offset = (Vector2){larguraTela * 0.5f, alturaTela * 0.5f};
        camera.zoom = ZOOM_CAMERA;
        const float metadeLarguraVisivel = larguraTela / (2.0f * camera.zoom);
        const float metadeAlturaVisivel = alturaTela / (2.0f * camera.zoom);
        camera.target.x = fmaxf(metadeLarguraVisivel, fminf(partida.jogador.x, MAPA_LARGURA - metadeLarguraVisivel));
        camera.target.y = fmaxf(metadeAlturaVisivel, fminf(partida.jogador.y, MAPA_ALTURA - metadeAlturaVisivel));

        const Vector2 posicaoJogadorNaTela = GetWorldToScreen2D(partida.jogador, camera);
        const Vector2 posicaoLuzShader = {
            posicaoJogadorNaTela.x,
            (float)alturaTela - posicaoJogadorNaTela.y
        };

        const float raioLuzShader = RaioLanterna(&partida.lanterna) * camera.zoom;

        SetShaderValue(shaderLanterna, localLuzShader, &posicaoLuzShader, SHADER_UNIFORM_VEC2);
        SetShaderValue(shaderLanterna, localRaioShader, &raioLuzShader, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shaderLanterna, localEscuridaoShader, &(float){INTENSIDADE_ESCURO}, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        BeginMode2D(camera);
        const Rectangle origemMapa = {0.0f, 0.0f, (float)mapa.width, (float)mapa.height};
        const Rectangle destinoMapa = {0.0f, 0.0f, (float)MAPA_LARGURA, (float)MAPA_ALTURA};
        DrawTexturePro(mapa, origemMapa, destinoMapa, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
            if (partida.bateriasColetadas[i]) continue;
            const float larguraBateria = TAMANHO_BATERIA_MAPA *
                (float)bateria.width / (float)bateria.height;
            const Rectangle origemBateria = {
                0.0f, 0.0f, (float)bateria.width, (float)bateria.height
            };
            const Rectangle destinoBateria = {
                partida.posicoesBateria[i].x - larguraBateria * 0.5f,
                partida.posicoesBateria[i].y - TAMANHO_BATERIA_MAPA * 0.5f,
                larguraBateria,
                TAMANHO_BATERIA_MAPA
            };
            DrawTexturePro(bateria, origemBateria, destinoBateria,
                           (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }

        const Texture2D spriteAtual = partida.olhandoVertical
            ? (partida.olhandoParaFrente
                ? (partida.quadroAnimacaoAndando ? spriteFrente : spriteParadoFrente)
                : (partida.quadroAnimacaoAndando ? spriteTras : spriteParadoTras))
            : (partida.olhandoEsquerda
                ? (partida.quadroAnimacaoAndando ? spriteAndandoEsquerda : spriteParadoEsquerda)
                : (partida.quadroAnimacaoAndando ? spriteAndando : spriteParado));
        DesenharJogador(partida.jogador, spriteAtual);

        EndMode2D();

        BeginShaderMode(shaderLanterna);
            DrawRectangle(0, 0, larguraTela, alturaTela, WHITE);
        EndShaderMode();

        DesenharHotbar(primeiroSlot, partida.slotSelecionado, bateria);
        DesenharCargaLanterna(&partida.lanterna, larguraTela);
        if (estado == ESTADO_PAUSADO) DesenharTelaPausa(larguraTela, alturaTela);

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
    UnloadTexture(bateria);
    LiberarMascaraParedes();
    UnloadTexture(telaInicial);
    UnloadTexture(mapa);
    CloseWindow();

    return codigoSaida;
}
