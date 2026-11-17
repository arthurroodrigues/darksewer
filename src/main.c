#include "main.h"
#include "som.h"
#include "colisao.h"
#include "barra_itens.h"
#include "menu.h"
#include "jogador.h"
#include "lanterna.h"
#include "inimigo.h"
#include "ranking.h"
#include "ordem_desenho.h"
#include "esconderijo.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define MAPA_ARQUIVO "assets/mapa.png"
#define TELA_INICIAL_ARQUIVO "assets/tela_de_inicio.png"
#define BATERIA_ARQUIVO "assets/bateria.png"
#define IMAGEM_ESCAPE_ARQUIVO "assets/imagem_escapou.png"
#define JUMPSCARE_ANIMACAO_ARQUIVO "assets/jumpscare.gif"
#define JUMPSCARE_AUDIO_ARQUIVO "assets/jumpscare.wav"
#define JUMPSCARE_FPS 15.0f
#define DURACAO_APAGAO 1.5f
#define DURACAO_FADE_ESCAPE 1.5f
#define DURACAO_EXIBICAO_ESCAPE 4.0f
#define INTERVALO_PISCADA_LANTERNA 0.3f
#define DURACAO_INTRODUCAO 1.8f
#define INTERVALO_LETRA_FALA 0.035f
#define INTERVALO_ANIMACAO 0.18f
#define ZOOM_CAMERA 12.0f
#define INTENSIDADE_ESCURO 0.99f
#define TAMANHO_ITEM_MAPA 9.0f
#define RAIO_COLETA_ITEM 10.0f
#define QUANTIDADE_CHAVES 3
#define QUANTIDADE_BATERIAS 3
#define QUANTIDADE_BUSSOLAS 1
#define QUANTIDADE_ITENS_MAPA \
    (QUANTIDADE_CHAVES + QUANTIDADE_BATERIAS + QUANTIDADE_BUSSOLAS)
#define RAIO_PORTA 16.0f
#define RAIO_BUEIRO 10.0f
#define VELOCIDADE_JOGADOR 90.0f
#define DURACAO_MAXIMA_ESCONDIDO 5.0f
#define RECARGA_ESCONDERIJO 8.0f
#define PROTECAO_SAIDA_ESCONDERIJO 1.5f
#define DISTANCIA_ESCONDERIJO_JOGADOR 30.0f
#define DISTANCIA_ENTRE_ESCONDERIJOS 45.0f
#define JOGADOR_INICIAL_X 100.0f
#define JOGADOR_INICIAL_Y 590.0f
#define ARQUIVO_SPRITE_PARADO "assets/jogador_parado.png"
#define ARQUIVO_SPRITE_ANDANDO "assets/jogador_andando.png"
#define ARQUIVO_SPRITE_PARADO_ESQUERDA "assets/jogador_parado_esquerda.png"
#define ARQUIVO_SPRITE_ANDANDO_ESQUERDA "assets/jogador_andando_esquerda.png"
#define ARQUIVO_SPRITE_FRENTE "assets/jogador_andando_para_frente.png"
#define ARQUIVO_SPRITE_PARADO_FRENTE "assets/jogador_parado_para_frente.png"
#define ARQUIVO_SPRITE_TRAS "assets/jogador_andando_para_tras.png"
#define ARQUIVO_SPRITE_PARADO_TRAS "assets/jogador_parado_para_tras.png"
#define LARGURA_JANELA 960
#define ALTURA_JANELA 720
#define ARQUIVO_RANKING "ranking.txt"

static const Rectangle kZonaPorta = {250.0f, 242.0f, 70.0f, 8.0f};
static const Vector2 kCentroPorta = {285.0f, 246.0f};
static const Vector2 kBueiro = {287.0f, 310.0f};
static const char *const kFalasIntroducao[] = {
    ": Caramba, eu to preso aqui!",
    "preciso fugir antes que esse palhaço me encontre",
    "Droga, a lanterna tá descarregando. preciso achar uma bateria rápido!"
};

typedef struct {
    Vector2 posicao;
    ItemHotbar item;
    bool coletado;
} ItemNoMapa;

static const struct {
    ItemHotbar item;
    int quantidade;
    float distanciaDoJogador;
    float distanciaEntreItens;
} kItensSorteados[] = {
    {ITEM_CHAVE, QUANTIDADE_CHAVES, 150.0f, 150.0f},
    {ITEM_BATERIA, QUANTIDADE_BATERIAS, 24.0f, 32.0f},
    {ITEM_BUSSOLA, QUANTIDADE_BUSSOLAS, 60.0f, 32.0f},
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
    ESTADO_INTRODUCAO,
    ESTADO_DIALOGO,
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_APAGAO,
    ESTADO_JUMPSCARE,
    ESTADO_TELA_ESCAPE,
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
    Esconderijo esconderijos[QUANTIDADE_ESCONDERIJOS];
    bool escondido;
    int indiceEscondido;
    Vector2 posicaoAntesDeEsconder;
    float tempoEscondido;
    float recargaEsconderijo;
    float protecaoSaida;
} Partida;

static Vector2 AlvoDaBussola(const Partida *partida){
    Vector2 alvo = partida->portaAberta ? kBueiro : kCentroPorta;
    float menorDistancia = -1.0f;

    for (int i = 0; i < QUANTIDADE_ITENS_MAPA; i++) {
        const ItemNoMapa *item = &partida->itens[i];
        if (item->item != ITEM_CHAVE || item->coletado) continue;

        const float distancia = Vector2DistanceSqr(partida->jogador, item->posicao);
        if (menorDistancia < 0.0f || distancia < menorDistancia) {
            menorDistancia = distancia;
            alvo = item->posicao;
        }
    }
    return alvo;
}

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
    Vector2 posicoesEsconderijos[QUANTIDADE_ITENS_MAPA + QUANTIDADE_ESCONDERIJOS];
    for (int i = 0; i < QUANTIDADE_ITENS_MAPA; i++) posicoesEsconderijos[i] = partida->itens[i].posicao;
    int totalOcupadas = QUANTIDADE_ITENS_MAPA;

    for (int i = 0; i < QUANTIDADE_ESCONDERIJOS; i++) {
        Vector2 posicao = {0};
        bool valida = false;
        for (int tentativa = 0; tentativa < 30 && !valida; tentativa++) {
            if (!EncontrarPosicaoProximaParede(
                    partida->jogador, DISTANCIA_ESCONDERIJO_JOGADOR, posicoesEsconderijos, totalOcupadas,
                    DISTANCIA_ENTRE_ESCONDERIJOS, &posicao)) return false;
            valida = PodeOcupar(posicao) &&
                     Vector2DistanceSqr(posicao, kBueiro) > 30.0f * 30.0f &&
                     Vector2DistanceSqr(posicao, kCentroPorta) > 35.0f * 35.0f;
        }
        if (!valida) return false;
        posicoesEsconderijos[totalOcupadas++] = posicao;
        partida->esconderijos[i] = (Esconderijo){
            posicao, i < QUANTIDADE_ARMARIOS ? ESCONDERIJO_ARMARIO : ESCONDERIJO_CAIXA
        };
    }

    return PosicionarInimigo(&partida->inimigo, partida->jogador);
}

static void EntrarNoEsconderijo(Partida *partida, int indice){
    partida->escondido = true;
    partida->indiceEscondido = indice;
    partida->tempoEscondido = 0.0f;
    partida->posicaoAntesDeEsconder = partida->jogador;
    partida->jogador = partida->esconderijos[indice].posicao;
}

static void SairDoEsconderijo(Partida *partida){
    partida->escondido = false;
    partida->jogador = partida->posicaoAntesDeEsconder;
    partida->recargaEsconderijo = RECARGA_ESCONDERIJO;
    partida->protecaoSaida = PROTECAO_SAIDA_ESCONDERIJO;
}

static int ContarCaracteresUTF8(const char *texto){
    int quantidade = 0;
    for (const unsigned char *byte = (const unsigned char *)texto; *byte != '\0'; byte++) {
        if ((*byte & 0xC0) != 0x80) quantidade++;
    }
    return quantidade;
}

static void CopiarPrefixoUTF8(char *destino, size_t tamanhoDestino,
                              const char *texto, int quantidadeCaracteres){
    size_t bytes = 0;
    int caracteres = 0;
    while (texto[bytes] != '\0' && caracteres < quantidadeCaracteres) {
        const unsigned char primeiroByte = (unsigned char)texto[bytes];
        const size_t tamanhoCaractere = primeiroByte < 0x80 ? 1 :
                                        primeiroByte < 0xE0 ? 2 :
                                        primeiroByte < 0xF0 ? 3 : 4;
        bytes += tamanhoCaractere;
        caracteres++;
    }
    if (bytes >= tamanhoDestino) bytes = tamanhoDestino - 1;
    for (size_t i = 0; i < bytes; i++) destino[i] = texto[i];
    destino[bytes] = '\0';
}

static void IniciarIntroducao(float *tempoIntroducao, int *indiceFala, float *tempoFala){
    *tempoIntroducao = 0.0f;
    *indiceFala = 0;
    *tempoFala = 0.0f;
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

    const bool pediuEsconderijo = IsKeyPressed(KEY_H) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (partida->escondido) {
        partida->tempoEscondido += deltaTempo;
        if (pediuEsconderijo || partida->tempoEscondido >= DURACAO_MAXIMA_ESCONDIDO) {
            SairDoEsconderijo(partida);
        }
    } else {
        partida->recargaEsconderijo = fmaxf(partida->recargaEsconderijo - deltaTempo, 0.0f);
        partida->protecaoSaida = fmaxf(partida->protecaoSaida - deltaTempo, 0.0f);
        if (pediuEsconderijo && partida->recargaEsconderijo <= 0.0f) {
            const int indice = EsconderijoPertoDoJogador(partida->esconderijos,
                                                         QUANTIDADE_ESCONDERIJOS, partida->jogador);
            if (indice >= 0) EntrarNoEsconderijo(partida, indice);
        }
    }

    Vector2 direcaoMovimento = {0.0f, 0.0f};

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) direcaoMovimento.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direcaoMovimento.x += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) direcaoMovimento.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direcaoMovimento.y += 1.0f;

    if (partida->escondido) direcaoMovimento = (Vector2){0.0f, 0.0f};
    const bool teclasMovimentoPressionadas = Vector2LengthSqr(direcaoMovimento) > 0.0f;
    const Vector2 posicaoAnteriorJogador = partida->jogador;

    if (teclasMovimentoPressionadas) {
        direcaoMovimento = Vector2Normalize(direcaoMovimento);
        partida->jogador = MoverComColisao(partida->jogador, Vector2Scale(direcaoMovimento, VELOCIDADE_JOGADOR * deltaTempo));
    }

    const bool jogadorEstaSeMovendo = Vector2DistanceSqr(partida->jogador, posicaoAnteriorJogador) > 0.0001f;

    for (int i = 0; i < QUANTIDADE_ITENS_MAPA && !partida->escondido; i++) {
        ItemNoMapa *item = &partida->itens[i];
        if (!item->coletado &&
            Vector2DistanceSqr(partida->jogador, item->posicao) <= RAIO_COLETA_ITEM * RAIO_COLETA_ITEM) {
            item->coletado = AdicionarItemHotbar(primeiroSlot, item->item);
            if (item->coletado) TocarSomColeta(sons);
        }
    }

    if (!partida->escondido && !partida->portaAberta &&
        CheckCollisionCircleRec(partida->jogador, RAIO_PORTA, kZonaPorta) &&
        ContarItemHotbar(primeiroSlot, ITEM_CHAVE) >= QUANTIDADE_CHAVES) {
        RemoverTodosDoItemHotbar(primeiroSlot, ITEM_CHAVE);
        partida->portaAberta = true;
        DefinirZonaBloqueada((Rectangle){0});
        TocarSomColeta(sons);
    }

    if (!partida->escondido && partida->portaAberta &&
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

    const bool jogadorInvisivel = partida->escondido || partida->protecaoSaida > 0.0f;
    AtualizarInimigo(&partida->inimigo, partida->jogador, LanternaAcesa(&partida->lanterna),
                     partida->escondido, deltaTempo);

    if (!jogadorInvisivel && InimigoPegouJogador(&partida->inimigo, partida->jogador)) {
        partida->pego = true;
    }

    AtualizarPassos(sons, jogadorEstaSeMovendo);
}

int main(void){
    ChangeDirectory(GetApplicationDirectory());

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

    SetTextureFilter(mapa, TEXTURE_FILTER_BILINEAR);

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
        !FileExists(BATERIA_ARQUIVO) || !FileExists("assets/caixa.png") ||
        !FileExists("assets/guarda_roupa.png") || !FileExists(JUMPSCARE_ANIMACAO_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Sprites, bateria, esconderijos ou animacao do jumpscare ausentes.");
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

    int quantidadeQuadrosJumpscare = 0;
    Image imagemJumpscare = LoadImageAnim(JUMPSCARE_ANIMACAO_ARQUIVO, &quantidadeQuadrosJumpscare);
    if (imagemJumpscare.data == NULL || imagemJumpscare.width <= 0 ||
        imagemJumpscare.height <= 0 || quantidadeQuadrosJumpscare <= 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar a animação do jumpscare.");
        if (imagemJumpscare.data != NULL) UnloadImage(imagemJumpscare);
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

    Texture2D texturaJumpscare = LoadTextureFromImage(imagemJumpscare);
    if (texturaJumpscare.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao criar a textura do jumpscare.");
        UnloadImage(imagemJumpscare);
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
    SetTextureFilter(texturaJumpscare, TEXTURE_FILTER_BILINEAR);
    const int bytesPorQuadroJumpscare = GetPixelDataSize(
        imagemJumpscare.width, imagemJumpscare.height, imagemJumpscare.format);

    Sons sonsJogo = {0};
    
    const bool audioDisponivel = InicializarSons(&sonsJogo);
    if (!audioDisponivel) {
        LiberarSons(&sonsJogo);
    } else {
        PlayMusicStream(sonsJogo.musicaFundo);
    }

    Sound somJumpscare = {0};
    if (audioDisponivel) {
        somJumpscare = LoadSound(JUMPSCARE_AUDIO_ARQUIVO);
        if (!IsSoundValid(somJumpscare)) {
            TraceLog(LOG_WARNING, "Nao foi possivel carregar o audio do jumpscare.");
        }
    }

    Shader shaderLanterna = LoadShaderFromMemory(NULL, kShaderLanterna);
    if (!IsShaderValid(shaderLanterna)) {
        TraceLog(LOG_ERROR, "Nao foi possivel criar o shader da lanterna.");
        if (IsSoundValid(somJumpscare)) UnloadSound(somJumpscare);
        LiberarSons(&sonsJogo);
        UnloadTexture(texturaJumpscare);
        UnloadImage(imagemJumpscare);
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
    float tempoIntroducao = 0.0f;
    int indiceFala = 0;
    float tempoFala = 0.0f;
    float tempoApagao = 0.0f;
    float tempoJumpscare = 0.0f;
    int quadroJumpscareAtual = -1;

    loot *primeiroSlot = CriarHotbar();
    if (primeiroSlot == NULL) {
        TraceLog(LOG_ERROR, "Falha ao criar a barra de itens.");
        UnloadShader(shaderLanterna);
        if (IsSoundValid(somJumpscare)) UnloadSound(somJumpscare);
        LiberarSons(&sonsJogo);
        UnloadTexture(texturaJumpscare);
        UnloadImage(imagemJumpscare);
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
    Texture2D texturaEscape = LoadTexture(IMAGEM_ESCAPE_ARQUIVO);
    if (texturaEscape.id == 0) {
        TraceLog(LOG_WARNING, "Nao foi possivel carregar a imagem da fuga: %s", IMAGEM_ESCAPE_ARQUIVO);
    } else {
        SetTextureFilter(texturaEscape, TEXTURE_FILTER_BILINEAR);
    }

    int codigoSaida = CarregarTexturasEsconderijo() ? 0 : 1;
    if (codigoSaida == 0 && !CarregarSpritesInimigo()) codigoSaida = 1;

    char caminhoRanking[512];
    snprintf(caminhoRanking, sizeof(caminhoRanking), "%s%s", GetApplicationDirectory(), ARQUIVO_RANKING);
    RegistroRanking *ranking = CarregarRanking(caminhoRanking);
    const RegistroRanking *registroNovo = NULL;
    char nomeJogador[TAMANHO_NOME_RANKING + 1] = "";
    int tamanhoNome = 0;
    bool digitacaoLiberada = false;
    float tempoTelaEscape = 0.0f;

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
                IniciarIntroducao(&tempoIntroducao, &indiceFala, &tempoFala);
                estado = ESTADO_INTRODUCAO;
                if (audioDisponivel) StopMusicStream(sonsJogo.musicaFundo);
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (estado == ESTADO_MENU && IsKeyPressed(KEY_R)) {
                estado = ESTADO_RANKING;
                registroNovo = NULL;
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (audioDisponivel && estado == ESTADO_MENU) UpdateMusicStream(sonsJogo.musicaFundo);

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
                IniciarIntroducao(&tempoIntroducao, &indiceFala, &tempoFala);
                estado = ESTADO_INTRODUCAO;
                if (audioDisponivel) StopMusicStream(sonsJogo.musicaFundo);
            } else if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) {
                estado = ESTADO_MENU;
            }

            if (audioDisponivel && estado == ESTADO_RANKING) UpdateMusicStream(sonsJogo.musicaFundo);

            BeginDrawing();
            DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
            DesenharRanking(ranking, registroNovo, larguraTela, alturaTela);
            EndDrawing();
            continue;
        }

        if (estado == ESTADO_INTRODUCAO) {
            tempoIntroducao += deltaTempo;
            if (tempoIntroducao >= DURACAO_INTRODUCAO) {
                estado = ESTADO_DIALOGO;
                tempoFala = 0.0f;
            }
        } else if (estado == ESTADO_DIALOGO) {
            const char *fala = kFalasIntroducao[indiceFala];
            const float duracaoFala = ContarCaracteresUTF8(fala) * INTERVALO_LETRA_FALA;
            if (IsKeyPressed(KEY_ENTER)) {
                if (indiceFala < (int)(sizeof(kFalasIntroducao) / sizeof(kFalasIntroducao[0])) - 1) {
                    indiceFala++;
                    tempoFala = 0.0f;
                } else if (tempoFala >= duracaoFala) {
                    estado = ESTADO_JOGANDO;
                } else {
                    tempoFala = duracaoFala;
                }
            }
            if (estado == ESTADO_DIALOGO) tempoFala = fminf(tempoFala + deltaTempo, duracaoFala);
        }

        if ((estado == ESTADO_JOGANDO || estado == ESTADO_PAUSADO) &&
            (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))) {
            estado = (estado == ESTADO_JOGANDO) ? ESTADO_PAUSADO : ESTADO_JOGANDO;
        }

        if (estado == ESTADO_JOGANDO) {
            AtualizarPartida(&partida, primeiroSlot, &sonsJogo, deltaTempo);
            if (partida.pego) {
                estado = ESTADO_JUMPSCARE;
                tempoJumpscare = 0.0f;
                quadroJumpscareAtual = -1;
                if (IsSoundValid(somJumpscare)) PlaySound(somJumpscare);
            } else if (partida.escapou) {
                estado = ESTADO_TELA_ESCAPE;
                tempoTelaEscape = 0.0f;
            } else if (partida.lanterna.carga <= 0.0f) {
                estado = ESTADO_APAGAO;
                tempoApagao = 0.0f;
            }
        } else if (estado == ESTADO_APAGAO) {
            AtualizarPartida(&partida, primeiroSlot, &sonsJogo, deltaTempo);
        } else {
            AtualizarPassos(&sonsJogo, false);
            if (estado == ESTADO_PAUSADO && IsKeyPressed(KEY_Q)) break;
            if (estado == ESTADO_VITORIA) {
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
                IniciarIntroducao(&tempoIntroducao, &indiceFala, &tempoFala);
                estado = ESTADO_INTRODUCAO;
            }
            if (estado != ESTADO_VITORIA && estado != ESTADO_APAGAO &&
                estado != ESTADO_JUMPSCARE && IsKeyPressed(KEY_M)) {
                estado = ESTADO_MENU;
                if (audioDisponivel) PlayMusicStream(sonsJogo.musicaFundo);
                BeginDrawing();
                DesenharTelaInicial(telaInicial, larguraTela, alturaTela, opacidadeTelaInicial);
                EndDrawing();
                continue;
            }
        }

        if (estado == ESTADO_APAGAO) {
            tempoApagao += deltaTempo;
            if (tempoApagao >= DURACAO_APAGAO) {
                estado = ESTADO_JUMPSCARE;
                tempoJumpscare = 0.0f;
                quadroJumpscareAtual = -1;
                if (IsSoundValid(somJumpscare)) PlaySound(somJumpscare);
            }
        }

        if (estado == ESTADO_JUMPSCARE) {
            tempoJumpscare += GetFrameTime();
            int quadro = (int)(tempoJumpscare * JUMPSCARE_FPS);
            if (quadro >= quantidadeQuadrosJumpscare) quadro = quantidadeQuadrosJumpscare - 1;
            if (quadro != quadroJumpscareAtual) {
                const unsigned char *dadosQuadro =
                    (const unsigned char *)imagemJumpscare.data +
                    (size_t)quadro * (size_t)bytesPorQuadroJumpscare;
                UpdateTexture(texturaJumpscare, dadosQuadro);
                quadroJumpscareAtual = quadro;
            }

            const float escala = fminf((float)larguraTela / texturaJumpscare.width,
                                       (float)alturaTela / texturaJumpscare.height);
            const float larguraVideo = (float)texturaJumpscare.width * escala;
            const float alturaVideo = (float)texturaJumpscare.height * escala;
            const Rectangle origemVideo = {
                0.0f, 0.0f, (float)texturaJumpscare.width, (float)texturaJumpscare.height
            };
            const Rectangle destinoVideo = {
                (larguraTela - larguraVideo) * 0.5f,
                (alturaTela - alturaVideo) * 0.5f,
                larguraVideo, alturaVideo
            };

            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(texturaJumpscare, origemVideo, destinoVideo,
                           (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
            EndDrawing();

            if (tempoJumpscare >= (float)quantidadeQuadrosJumpscare / JUMPSCARE_FPS) {
                estado = ESTADO_FIM;
                if (IsSoundValid(somJumpscare) && IsSoundPlaying(somJumpscare)) {
                    StopSound(somJumpscare);
                }
            }
            continue;
        }

        if (estado == ESTADO_TELA_ESCAPE) {
            tempoTelaEscape += deltaTempo;
            const float opacidade = fminf(tempoTelaEscape / DURACAO_FADE_ESCAPE, 1.0f);

            BeginDrawing();
            ClearBackground(BLACK);
            if (texturaEscape.id != 0) {
                const float escala = fminf((float)larguraTela / texturaEscape.width,
                                           (float)alturaTela / texturaEscape.height);
                const float larguraImagem = (float)texturaEscape.width * escala;
                const float alturaImagem = (float)texturaEscape.height * escala;
                const Rectangle origem = {
                    0.0f, 0.0f, (float)texturaEscape.width, (float)texturaEscape.height
                };
                const Rectangle destino = {
                    (larguraTela - larguraImagem) * 0.5f,
                    (alturaTela - alturaImagem) * 0.5f,
                    larguraImagem, alturaImagem
                };
                DrawTexturePro(texturaEscape, origem, destino, (Vector2){0.0f, 0.0f}, 0.0f,
                               (Color){255, 255, 255, (unsigned char)(255.0f * opacidade)});
            }
            EndDrawing();

            if (tempoTelaEscape >= DURACAO_FADE_ESCAPE + DURACAO_EXIBICAO_ESCAPE) {
                estado = ESTADO_VITORIA;
                nomeJogador[0] = '\0';
                tamanhoNome = 0;
                digitacaoLiberada = false;
            }
            continue;
        }

        camera.offset = (Vector2){larguraTela * 0.5f, alturaTela * 0.5f};
        camera.zoom = ZOOM_CAMERA;
        const float metadeLarguraVisivel = larguraTela / (2.0f * camera.zoom);
        const float metadeAlturaVisivel = alturaTela / (2.0f * camera.zoom);
        camera.target.x = fmaxf(metadeLarguraVisivel, fminf(partida.jogador.x, MAPA_LARGURA - metadeLarguraVisivel));
        camera.target.y = fmaxf(metadeAlturaVisivel, fminf(partida.jogador.y, MAPA_ALTURA - metadeAlturaVisivel));

        const Vector2 centroJogadorNoMundo = {partida.jogador.x, partida.jogador.y - 9.0f};
        const Vector2 posicaoJogadorNaTela = GetWorldToScreen2D(centroJogadorNoMundo, camera);
        const Vector2 posicaoLuzShader = {
            posicaoJogadorNaTela.x,
            (float)alturaTela - posicaoJogadorNaTela.y
        };

        const Lanterna lanternaPiscando = {.carga = 0.0f, .ligada = true};
        const float raioLuzShader =
            (estado == ESTADO_APAGAO
                ? RaioLanterna(&lanternaPiscando)
                : RaioLanterna(&partida.lanterna)) * camera.zoom;

        SetShaderValue(shaderLanterna, localLuzShader, &posicaoLuzShader, SHADER_UNIFORM_VEC2);
        SetShaderValue(shaderLanterna, localRaioShader, &raioLuzShader, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shaderLanterna, localEscuridaoShader, &(float){INTENSIDADE_ESCURO}, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        BeginMode2D(camera);
        const Rectangle origemMapa = {0.0f, 0.0f, (float)mapa.width, (float)mapa.height};
        const Rectangle destinoMapa = {0.0f, 0.0f, (float)MAPA_LARGURA, (float)MAPA_ALTURA};
        DrawTexturePro(mapa, origemMapa, destinoMapa, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        if (!partida.portaAberta) {
            DrawRectangleRec((Rectangle){kZonaPorta.x, kZonaPorta.y + 2.0f,
                                         kZonaPorta.width, kZonaPorta.height - 4.0f}, (Color){28, 30, 32, 235});
            for (float x = kZonaPorta.x + 1.0f; x < kZonaPorta.x + kZonaPorta.width; x += 4.0f) {
                DrawRectangleRec((Rectangle){x, kZonaPorta.y, 1.0f, kZonaPorta.height}, (Color){96, 102, 106, 255});
            }
        }

        const Texture2D spriteAtual = partida.olhandoVertical
            ? (partida.olhandoParaFrente
                ? (partida.quadroAnimacaoAndando ? spriteFrente : spriteParadoFrente)
                : (partida.quadroAnimacaoAndando ? spriteTras : spriteParadoTras))
            : (partida.olhandoEsquerda
                ? (partida.quadroAnimacaoAndando ? spriteAndandoEsquerda : spriteParadoEsquerda)
                : (partida.quadroAnimacaoAndando ? spriteAndando : spriteParado));

        ElementoDesenho elementos[QUANTIDADE_ITENS_MAPA + QUANTIDADE_ESCONDERIJOS + 2];
        int quantidadeElementos = 0;
        const float baseItem = TAMANHO_ITEM_MAPA * 0.5f;

        for (int i = 0; i < QUANTIDADE_ITENS_MAPA; i++) {
            if (partida.itens[i].coletado) continue;
            elementos[quantidadeElementos++] =
                (ElementoDesenho){partida.itens[i].posicao.y + baseItem, DESENHO_ITEM, i};
        }
        for (int i = 0; i < QUANTIDADE_ESCONDERIJOS; i++) {
            elementos[quantidadeElementos++] =
                (ElementoDesenho){partida.esconderijos[i].posicao.y, DESENHO_ESCONDERIJO, i};
        }
        elementos[quantidadeElementos++] = (ElementoDesenho){partida.inimigo.posicao.y, DESENHO_INIMIGO, 0};
        if (!partida.escondido) {
            elementos[quantidadeElementos++] = (ElementoDesenho){partida.jogador.y, DESENHO_JOGADOR, 0};
        }

        OrdenarPorProfundidade(elementos, quantidadeElementos);

        for (int i = 0; i < quantidadeElementos; i++) {
            switch (elementos[i].tipo) {
                case DESENHO_ITEM: {
                    const ItemNoMapa *item = &partida.itens[elementos[i].indice];
                    DesenharItem(item->item, item->posicao, TAMANHO_ITEM_MAPA, bateria);
                } break;
                case DESENHO_ESCONDERIJO:
                    DesenharEsconderijo(&partida.esconderijos[elementos[i].indice]);
                    break;
                case DESENHO_INIMIGO: DesenharInimigo(&partida.inimigo); break;
                case DESENHO_JOGADOR: DesenharJogador(partida.jogador, spriteAtual); break;
            }
        }

        EndMode2D();

        BeginShaderMode(shaderLanterna);
            DrawRectangle(0, 0, larguraTela, alturaTela, WHITE);
        EndShaderMode();

        if (estado == ESTADO_APAGAO &&
            fmodf(tempoApagao, INTERVALO_PISCADA_LANTERNA) >= INTERVALO_PISCADA_LANTERNA * 0.5f) {
            DrawRectangle(0, 0, larguraTela, alturaTela, BLACK);
        } else if (estado == ESTADO_INTRODUCAO) {
            const float progresso = fminf(tempoIntroducao / DURACAO_INTRODUCAO, 1.0f);
            DrawRectangle(0, 0, larguraTela, alturaTela,
                          (Color){0, 0, 0, (unsigned char)(255.0f * (1.0f - progresso))});
        } else if (estado == ESTADO_DIALOGO) {
            const char *fala = kFalasIntroducao[indiceFala];
            char falaVisivel[256];
            const int caracteresVisiveis = (int)(tempoFala / INTERVALO_LETRA_FALA);
            CopiarPrefixoUTF8(falaVisivel, sizeof(falaVisivel), fala, caracteresVisiveis);
            const bool ultimaFala = indiceFala == (int)(sizeof(kFalasIntroducao) / sizeof(kFalasIntroducao[0])) - 1;
            const bool falaCompleta = tempoFala >= ContarCaracteresUTF8(fala) * INTERVALO_LETRA_FALA;
            DesenharCaixaDialogo(falaVisivel, fala, ultimaFala, falaCompleta,
                                 larguraTela, alturaTela);
        }

        if (estado == ESTADO_JOGANDO && partida.slotSelecionado->item == ITEM_BUSSOLA) {
            const Vector2 centro = GetWorldToScreen2D(centroJogadorNoMundo, camera);
            const Vector2 alvo = GetWorldToScreen2D(AlvoDaBussola(&partida), camera);
            const Vector2 direcao = Vector2Normalize(Vector2Subtract(alvo, centro));
            const Vector2 lateral = {-direcao.y, direcao.x};
            const Vector2 base = Vector2Add(centro, Vector2Scale(direcao, 78.0f));
            DrawTriangle(Vector2Add(centro, Vector2Scale(direcao, 104.0f)),
                         Vector2Subtract(base, Vector2Scale(lateral, 11.0f)),
                         Vector2Add(base, Vector2Scale(lateral, 11.0f)), (Color){212, 175, 55, 255});
        }

        if (estado != ESTADO_INTRODUCAO && estado != ESTADO_DIALOGO &&
            estado != ESTADO_APAGAO) {
            DesenharHotbar(primeiroSlot, partida.slotSelecionado, bateria);
            DesenharCargaLanterna(&partida.lanterna, larguraTela);

            const char *relogio = FormatarTempo(partida.tempoPartida);
            DrawText(relogio, (larguraTela - MeasureText(relogio, 24)) / 2, 20, 24, (Color){214, 204, 190, 255});

            const int chavesNaMao = ContarItemHotbar(primeiroSlot, ITEM_CHAVE);
            const char *objetivo = partida.portaAberta
                ? "Escape pelo poço"
                : (chavesNaMao >= QUANTIDADE_CHAVES
                    ? "Leve as chaves até a grade do poço, na sala central"
                    : TextFormat("Encontre as chaves: %d/%d", chavesNaMao, QUANTIDADE_CHAVES));
            DrawText(objetivo, (larguraTela - MeasureText(objetivo, 20)) / 2, alturaTela - 36, 20,
                     (Color){150, 112, 72, 255});

            if (estado == ESTADO_JOGANDO && partida.escondido) {
                const float restante = DURACAO_MAXIMA_ESCONDIDO - partida.tempoEscondido;
                const char *aviso = TextFormat("Escondido: %.1fs  (H para sair)", fmaxf(restante, 0.0f));
                const int larguraBarra = 240;
                const int xBarra = (larguraTela - larguraBarra) / 2;
                DrawText(aviso, (larguraTela - MeasureText(aviso, 22)) / 2, alturaTela - 100, 22,
                         (Color){214, 204, 190, 255});
                DrawRectangle(xBarra, alturaTela - 70, larguraBarra, 10, (Color){10, 16, 15, 200});
                DrawRectangle(xBarra, alturaTela - 70,
                              (int)((float)larguraBarra * fmaxf(restante, 0.0f) / DURACAO_MAXIMA_ESCONDIDO),
                              10, (Color){150, 112, 72, 255});
                DrawRectangleLinesEx((Rectangle){xBarra, alturaTela - 70, larguraBarra, 10}, 2,
                                     (Color){74, 88, 82, 255});
            } else if (estado == ESTADO_JOGANDO &&
                       EsconderijoPertoDoJogador(partida.esconderijos, QUANTIDADE_ESCONDERIJOS,
                                                 partida.jogador) >= 0) {
                const char *aviso = partida.recargaEsconderijo > 0.0f
                    ? TextFormat("Esconderijo disponível em %.0fs", ceilf(partida.recargaEsconderijo))
                    : "H - Esconder-se";
                DrawText(aviso, (larguraTela - MeasureText(aviso, 24)) / 2, alturaTela - 70, 24,
                         (Color){214, 204, 190, 255});
            }

            if (estado == ESTADO_JOGANDO && !partida.portaAberta &&
                CheckCollisionCircleRec(partida.jogador, RAIO_PORTA, kZonaPorta)) {
                const int faltam = QUANTIDADE_CHAVES - ContarItemHotbar(primeiroSlot, ITEM_CHAVE);
                const char *aviso = faltam == 1 ? "Falta 1 chave" : TextFormat("Faltam %d chaves", faltam);
                DrawText(aviso, (larguraTela - MeasureText(aviso, 24)) / 2, alturaTela - 70, 24,
                         (Color){214, 204, 190, 255});
            }
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
    LiberarTexturasEsconderijo();
    LiberarHotbar(primeiroSlot);
    if (texturaEscape.id != 0) UnloadTexture(texturaEscape);
    UnloadShader(shaderLanterna);
    if (IsSoundValid(somJumpscare)) UnloadSound(somJumpscare);
    LiberarSons(&sonsJogo);
    UnloadTexture(texturaJumpscare);
    UnloadImage(imagemJumpscare);
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
