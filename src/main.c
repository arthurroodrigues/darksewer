#include "main.h"
#include "som.h"
#include "colisao.h"
#include "barra_itens.h"
#include "menu.h"
#include "jogador.h"
#include "raymath.h"
#include <math.h>
#include <time.h>

#define MAPA_ARQUIVO "assets\\mapa.png"
#define TELA_INICIAL_ARQUIVO "assets\\tela_de_inicio.png"
#define BATERIA_ARQUIVO "assets\\bateria.png"
#define INTERVALO_ANIMACAO 0.18f
#define ZOOM_CAMERA 7.5f
#define RAIO_LANTERNA 70.0f
#define INTENSIDADE_ESCURO 0.99f
#define TAMANHO_BATERIA_MAPA 15.0f
#define RAIO_COLETA_BATERIA 12.5f
#define DISTANCIA_MINIMA_SPAWN_BATERIA 96.0f
#define DISTANCIA_MINIMA_ENTRE_BATERIAS 160.0f
#define QUANTIDADE_BATERIAS 3
#define VELOCIDADE_JOGADOR 112.0f
#define JOGADOR_INICIAL_X 96.0f
#define JOGADOR_INICIAL_Y 816.0f
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

    SetRandomSeed((unsigned int)time(NULL) ^ (unsigned int)(GetTime() * 1000000.0));
    Vector2 posicoesBateria[QUANTIDADE_BATERIAS] = {0};
    for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
        if (!EncontrarPosicaoAcessivelAleatoria(
                jogador, DISTANCIA_MINIMA_SPAWN_BATERIA,
                posicoesBateria, i, DISTANCIA_MINIMA_ENTRE_BATERIAS,
                &posicoesBateria[i])) {
            LiberarMascaraParedes();
            UnloadTexture(telaInicial);
            UnloadTexture(mapa);
            CloseWindow();
            return 1;
        }
    }

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
    loot *slotSelecionado = primeiroSlot;
    bool bateriasColetadas[QUANTIDADE_BATERIAS] = {false};

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
                if (audioDisponivel) StopMusicStream(sonsJogo.musicaFundo);
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (audioDisponivel && !jogoIniciado) UpdateMusicStream(sonsJogo.musicaFundo);

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

        for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
            if (!bateriasColetadas[i] &&
                Vector2DistanceSqr(jogador, posicoesBateria[i]) <=
                    RAIO_COLETA_BATERIA * RAIO_COLETA_BATERIA) {
                bateriasColetadas[i] =
                    AdicionarItemHotbar(primeiroSlot, ITEM_BATERIA);
                if (bateriasColetadas[i]) TocarSomColeta(&sonsJogo);
            }
        }

        if (jogadorEstaSeMovendo) {
            const float intensidadeHorizontal = fabsf(direcaoMovimento.x);
            const float intensidadeVertical = fabsf(direcaoMovimento.y);

            if (intensidadeVertical > intensidadeHorizontal ||
                (intensidadeVertical == intensidadeHorizontal && olhandoVertical)) {
                olhandoParaFrente = direcaoMovimento.y > 0.0f;
                olhandoVertical = true;
            } else {
                olhandoEsquerda = direcaoMovimento.x < 0.0f;
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
        for (int i = 0; i < QUANTIDADE_BATERIAS; i++) {
            if (bateriasColetadas[i]) continue;
            const float larguraBateria = TAMANHO_BATERIA_MAPA *
                (float)bateria.width / (float)bateria.height;
            const Rectangle origemBateria = {
                0.0f, 0.0f, (float)bateria.width, (float)bateria.height
            };
            const Rectangle destinoBateria = {
                posicoesBateria[i].x - larguraBateria * 0.5f,
                posicoesBateria[i].y - TAMANHO_BATERIA_MAPA * 0.5f,
                larguraBateria,
                TAMANHO_BATERIA_MAPA
            };
            DrawTexturePro(bateria, origemBateria, destinoBateria,
                           (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }

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

        DesenharHotbar(primeiroSlot, slotSelecionado, bateria);

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

    return 0;
}