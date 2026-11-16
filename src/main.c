#include "main.h"
#include "som.h"
#include "colisao.h"
#include "barra_itens.h"
#include "menu.h"
#include "jogador.h"
#include "lanterna.h"
#include "inimigo.h"
#include "ranking.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define MAPA_ARQUIVO "assets\\mapa.png"
#define TELA_INICIAL_ARQUIVO "assets\\tela_de_inicio.png"
#define BATERIA_ARQUIVO "assets\\bateria.png"
#define INTERVALO_ANIMACAO 0.18f
#define ZOOM_CAMERA 7.5f
#define INTENSIDADE_ESCURO 0.99f
#define TAMANHO_ITEM_MAPA 13.0f
#define RAIO_COLETA_ITEM 12.5f
#define QUANTIDADE_CHAVES 3
#define QUANTIDADE_BATERIAS 3
#define QUANTIDADE_ITENS_MAPA (QUANTIDADE_CHAVES + QUANTIDADE_BATERIAS)
#define RAIO_PORTA 16.0f
#define RAIO_BUEIRO 9.0f
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
#define ARQUIVO_RANKING "ranking.txt"

// porta da sala do bueiro, por onde o jogador escapa
static const Rectangle kZonaPorta = {478.0f, 502.0f, 28.0f, 8.0f};
static const Vector2 kCentroPorta = {492.0f, 506.0f};
static const Vector2 kBueiro = {491.0f, 585.0f};

typedef struct {
    Vector2 posicao;
    ItemHotbar item;
    bool coletado;
} ItemNoMapa;

// o que nasce no mapa a cada partida: quantos, a que distancia do jogador e entre si
static const struct {
    ItemHotbar item;
    int quantidade;
    float distanciaDoJogador;
    float distanciaEntreItens;
} kItensSorteados[] = {
    {ITEM_CHAVE, QUANTIDADE_CHAVES, 150.0f, 150.0f},
    {ITEM_BATERIA, QUANTIDADE_BATERIAS, 24.0f, 32.0f},
};

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
    ESTADO_PAUSADO,
    ESTADO_FIM,
    ESTADO_VITORIA,
    ESTADO_RANKING
} EstadoJogo;

typedef struct {
    Vector2 jogador;
    ItemNoMapa itens[QUANTIDADE_ITENS_MAPA];
    bool portaAberta;
    bool escapou;
    Lanterna lanterna;
    Inimigo inimigo;
    bool pego;
    float tempoPartida;
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
    DefinirZonaBloqueada(kZonaPorta);

    Vector2 posicoesOcupadas[QUANTIDADE_ITENS_MAPA];
    int total = 0;
    for (size_t tipo = 0; tipo < sizeof(kItensSorteados) / sizeof(kItensSorteados[0]); tipo++) {
        for (int i = 0; i < kItensSorteados[tipo].quantidade && total < QUANTIDADE_ITENS_MAPA; i++) {
            if (!EncontrarPosicaoAcessivelAleatoria(
                    partida->jogador, kItensSorteados[tipo].distanciaDoJogador,
                    posicoesOcupadas, total, kItensSorteados[tipo].distanciaEntreItens,
                    &posicoesOcupadas[total])) return false;
            partida->itens[total] = (ItemNoMapa){posicoesOcupadas[total], kItensSorteados[tipo].item, false};
            total++;
        }
    }
    return PosicionarInimigo(&partida->inimigo, partida->jogador);
}

static void AtualizarPartida(Partida *partida, loot *primeiroSlot, Sons *sons, float deltaTempo){
    partida->tempoPartida += deltaTempo;

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

    for (int i = 0; i < QUANTIDADE_ITENS_MAPA; i++) {
        ItemNoMapa *item = &partida->itens[i];
        if (!item->coletado &&
            Vector2DistanceSqr(partida->jogador, item->posicao) <= RAIO_COLETA_ITEM * RAIO_COLETA_ITEM) {
            item->coletado = AdicionarItemHotbar(primeiroSlot, item->item);
            if (item->coletado) TocarSomColeta(sons);
        }
    }

    if (!partida->portaAberta &&
        Vector2DistanceSqr(partida->jogador, kCentroPorta) <= RAIO_PORTA * RAIO_PORTA &&
        ContarItemHotbar(primeiroSlot, ITEM_CHAVE) >= QUANTIDADE_CHAVES) {
        RemoverTodosDoItemHotbar(primeiroSlot, ITEM_CHAVE);
        partida->portaAberta = true;
        DefinirZonaBloqueada((Rectangle){0});
        TocarSomColeta(sons);
    }

    if (partida->portaAberta &&
        Vector2DistanceSqr(partida->jogador, kBueiro) <= RAIO_BUEIRO * RAIO_BUEIRO) {
        partida->escapou = true;
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

    AtualizarInimigo(&partida->inimigo, partida->jogador,
                     LanternaAcesa(&partida->lanterna), deltaTempo);

    if (InimigoPegouJogador(&partida->inimigo, partida->jogador)) partida->pego = true;

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
    int codigoSaida = CarregarSpritesInimigo() ? 0 : 1;

    char caminhoRanking[512];
    snprintf(caminhoRanking, sizeof(caminhoRanking), "%s%s", GetApplicationDirectory(), ARQUIVO_RANKING);
    RegistroRanking *ranking = CarregarRanking(caminhoRanking);
    const RegistroRanking *registroNovo = NULL;
    char nomeJogador[TAMANHO_NOME_RANKING + 1] = "";
    int tamanhoNome = 0;
    bool digitacaoLiberada = false;

    while (codigoSaida == 0 && !WindowShouldClose()) {
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

            if (estado == ESTADO_MENU && IsKeyPressed(KEY_R)) {
                estado = ESTADO_RANKING;
                registroNovo = NULL;
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (audioDisponivel && estado != ESTADO_JOGANDO) UpdateMusicStream(sonsJogo.musicaFundo);

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            EndDrawing();
            continue;
        }

        if (estado == ESTADO_RANKING) {
            if (IsKeyPressed(KEY_ENTER)) {
                if (!IniciarPartida(&partida, primeiroSlot)) {
                    codigoSaida = 1;
                    break;
                }
                estado = ESTADO_JOGANDO;
                if (audioDisponivel) StopMusicStream(sonsJogo.musicaFundo);
            } else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
                estado = ESTADO_MENU;
            }

            if (audioDisponivel && estado != ESTADO_JOGANDO) UpdateMusicStream(sonsJogo.musicaFundo);

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            DesenharRanking(ranking, registroNovo, larguraTela, alturaTela);
            EndDrawing();
            continue;
        }

        if ((estado == ESTADO_JOGANDO || estado == ESTADO_PAUSADO) &&
            (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))) {
            estado = (estado == ESTADO_JOGANDO) ? ESTADO_PAUSADO : ESTADO_JOGANDO;
        }

        if (estado == ESTADO_JOGANDO) {
            AtualizarPartida(&partida, primeiroSlot, &sonsJogo, deltaTempo);
            if (partida.pego) estado = ESTADO_FIM;
            else if (partida.escapou) {
                estado = ESTADO_VITORIA;
                nomeJogador[0] = '\0';
                tamanhoNome = 0;
                digitacaoLiberada = false;
            }
        } else {
            AtualizarPassos(&sonsJogo, false);
            if (estado == ESTADO_PAUSADO && IsKeyPressed(KEY_Q)) break;
            if (estado == ESTADO_VITORIA) {
                // so comeca a digitar depois de soltar as teclas de andar, para elas nao entrarem no nome
                if (!digitacaoLiberada) {
                    digitacaoLiberada = !IsKeyDown(KEY_W) && !IsKeyDown(KEY_A) &&
                                        !IsKeyDown(KEY_S) && !IsKeyDown(KEY_D);
                }

                for (int letra = GetCharPressed(); letra > 0; letra = GetCharPressed()) {
                    if (letra >= 'a' && letra <= 'z') letra -= 'a' - 'A';
                    const bool letraValida = (letra >= 'A' && letra <= 'Z') ||
                                             (letra >= '0' && letra <= '9') || letra == ' ';
                    if (digitacaoLiberada && letraValida && tamanhoNome < TAMANHO_NOME_RANKING) {
                        nomeJogador[tamanhoNome++] = (char)letra;
                        nomeJogador[tamanhoNome] = '\0';
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE) && tamanhoNome > 0) nomeJogador[--tamanhoNome] = '\0';

                if (IsKeyPressed(KEY_ENTER)) {
                    registroNovo = InserirNoRanking(&ranking, tamanhoNome > 0 ? nomeJogador : "ANONIMO",
                                                    partida.tempoPartida);
                    if (!SalvarRanking(ranking, caminhoRanking)) {
                        TraceLog(LOG_WARNING, "Nao foi possivel salvar o ranking em %s", caminhoRanking);
                    }
                    estado = ESTADO_RANKING;
                    if (audioDisponivel) PlayMusicStream(sonsJogo.musicaFundo);
                    BeginDrawing();
                    DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
                    DesenharRanking(ranking, registroNovo, larguraTela, alturaTela);
                    EndDrawing();
                    continue;
                }
            }
            if (estado == ESTADO_FIM && IsKeyPressed(KEY_ENTER)) {
                if (!IniciarPartida(&partida, primeiroSlot)) {
                    codigoSaida = 1;
                    break;
                }
                estado = ESTADO_JOGANDO;
            }
            if (estado != ESTADO_VITORIA && IsKeyPressed(KEY_M)) {
                estado = ESTADO_MENU;
                if (audioDisponivel) PlayMusicStream(sonsJogo.musicaFundo);
                BeginDrawing();
                DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
                EndDrawing();
                continue;
            }
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
        for (int i = 0; i < QUANTIDADE_ITENS_MAPA; i++) {
            const ItemNoMapa *item = &partida.itens[i];
            if (!item->coletado) DesenharItem(item->item, item->posicao, TAMANHO_ITEM_MAPA, bateria);
        }

        if (!partida.portaAberta) {
            // grade de ferro sobre a porta trancada
            DrawRectangleRec((Rectangle){479.0f, 503.0f, 26.0f, 5.0f}, (Color){28, 30, 32, 235});
            for (float x = 480.0f; x < 505.0f; x += 4.0f) {
                DrawRectangleRec((Rectangle){x, 502.0f, 1.0f, 7.0f}, (Color){96, 102, 106, 255});
            }
        }

        // quem esta mais abaixo na tela fica na frente
        const bool inimigoAtras = partida.inimigo.posicao.y <= partida.jogador.y;
        if (inimigoAtras) DesenharInimigo(&partida.inimigo);

        const Texture2D spriteAtual = partida.olhandoVertical
            ? (partida.olhandoParaFrente
                ? (partida.quadroAnimacaoAndando ? spriteFrente : spriteParadoFrente)
                : (partida.quadroAnimacaoAndando ? spriteTras : spriteParadoTras))
            : (partida.olhandoEsquerda
                ? (partida.quadroAnimacaoAndando ? spriteAndandoEsquerda : spriteParadoEsquerda)
                : (partida.quadroAnimacaoAndando ? spriteAndando : spriteParado));
        DesenharJogador(partida.jogador, spriteAtual);
        if (!inimigoAtras) DesenharInimigo(&partida.inimigo);

        EndMode2D();

        BeginShaderMode(shaderLanterna);
            DrawRectangle(0, 0, larguraTela, alturaTela, WHITE);
        EndShaderMode();

        DesenharHotbar(primeiroSlot, partida.slotSelecionado, bateria);
        DesenharCargaLanterna(&partida.lanterna, larguraTela);

        const char *relogio = FormatarTempo(partida.tempoPartida);
        DrawText(relogio, (larguraTela - MeasureText(relogio, 24)) / 2, 20, 24, (Color){214, 204, 190, 255});

        if (estado == ESTADO_JOGANDO && !partida.portaAberta &&
            Vector2DistanceSqr(partida.jogador, kCentroPorta) <= RAIO_PORTA * RAIO_PORTA) {
            const int faltam = QUANTIDADE_CHAVES - ContarItemHotbar(primeiroSlot, ITEM_CHAVE);
            const char *aviso = faltam == 1 ? "Falta 1 chave" : TextFormat("Faltam %d chaves", faltam);
            DrawText(aviso, (larguraTela - MeasureText(aviso, 24)) / 2, alturaTela - 70, 24,
                     (Color){214, 204, 190, 255});
        }

        if (estado == ESTADO_PAUSADO) DesenharTelaPausa(larguraTela, alturaTela);
        if (estado == ESTADO_FIM) DesenharTelaFim(larguraTela, alturaTela);
        if (estado == ESTADO_VITORIA) {
            DesenharTelaVitoria(larguraTela, alturaTela, partida.tempoPartida, nomeJogador);
        }

        EndDrawing();
    }

    LiberarRanking(ranking);
    LiberarSpritesInimigo();
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
