#include "inimigo.h"
#include "colisao.h"
#include "jogador.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

#define VELOCIDADE_PATRULHA 34.0f
#define VELOCIDADE_PERSEGUICAO 60.0f
#define ALCANCE_VISAO_LANTERNA_ACESA 110.0f
#define ALCANCE_VISAO_ESCURO 35.0f
#define RAIO_CAPTURA 8.0f
#define INTERVALO_RECALCULO_CAMINHO 0.4f
#define TEMPO_PROCURANDO 3.0f
#define DISTANCIA_MINIMA_NASCIMENTO 250.0f
#define DISTANCIA_MINIMA_PATRULHA 80.0f
#define INTERVALO_ANIMACAO_INIMIGO 0.18f
#define ALTURA_SPRITE_INIMIGO 20.0f
#define QUANTIDADE_SPRITES_INIMIGO 4

// As imagens novas têm uma pose por direção. A mesma pose é usada parado e andando.
static const char *const kArquivosSprites[QUANTIDADE_SPRITES_INIMIGO] = {
    "assets/inimigo_sprite_direita.png",
    "assets/inimigo_sprite_esquerda.png",
    "assets/inimigo_sprite_frente.png",
    "assets/inimigo_sprite_tras.png"
};
static Texture2D sprites[QUANTIDADE_SPRITES_INIMIGO];

bool CarregarSpritesInimigo(void){
    for (int i = 0; i < QUANTIDADE_SPRITES_INIMIGO; i++) {
        sprites[i] = LoadTexture(kArquivosSprites[i]);
        if (sprites[i].id == 0) {
            TraceLog(LOG_ERROR, "Falha ao carregar o sprite do inimigo: %s", kArquivosSprites[i]);
            LiberarSpritesInimigo();
            return false;
        }
        SetTextureFilter(sprites[i], TEXTURE_FILTER_BILINEAR);
    }
    return true;
}

void LiberarSpritesInimigo(void){
    for (int i = 0; i < QUANTIDADE_SPRITES_INIMIGO; i++) {
        if (sprites[i].id != 0) UnloadTexture(sprites[i]);
        sprites[i] = (Texture2D){0};
    }
}

void DesenharInimigo(const Inimigo *inimigo){
    const int direcao = inimigo->olhandoVertical
        ? (inimigo->olhandoParaFrente ? 2 : 3)
        : (inimigo->olhandoEsquerda ? 1 : 0);
    DesenharPersonagem(inimigo->posicao, sprites[direcao], ALTURA_SPRITE_INIMIGO, false);
}

static void EscolherAlvoPatrulha(Inimigo *inimigo){
    Vector2 alvo;

    inimigo->estado = INIMIGO_PATRULHANDO;
    if (EncontrarPosicaoAcessivelAleatoria(inimigo->posicao, DISTANCIA_MINIMA_PATRULHA,
                                           NULL, 0, 0.0f, &alvo)) {
        AtualizarMapaDistancias(alvo);
    }
}

static void Andar(Inimigo *inimigo, float distancia, float deltaTempo){
    const Vector2 posicaoAnterior = inimigo->posicao;

    for (int i = 0; i < 4 && distancia > 0.0f; i++) {
        const Vector2 destino = ProximoPontoNoCaminho(inimigo->posicao);
        const float falta = Vector2Distance(inimigo->posicao, destino);
        if (falta < 0.001f) break;

        const float passo = fminf(distancia, falta);
        inimigo->posicao = Vector2MoveTowards(inimigo->posicao, destino, passo);
        distancia -= passo;
    }

    const Vector2 movimento = Vector2Subtract(inimigo->posicao, posicaoAnterior);
    if (Vector2LengthSqr(movimento) < 0.0001f) {
        inimigo->tempoAnimacaoAndando = 0.0f;
        inimigo->quadroAnimacaoAndando = false;
        return;
    }

    const float intensidadeHorizontal = fabsf(movimento.x);
    const float intensidadeVertical = fabsf(movimento.y);

    bool olhandoVertical = inimigo->olhandoVertical;
    if (olhandoVertical && intensidadeHorizontal > intensidadeVertical * 1.25f) {
        olhandoVertical = false;
    } else if (!olhandoVertical && intensidadeVertical > intensidadeHorizontal * 1.25f) {
        olhandoVertical = true;
    }

    if (olhandoVertical) {
        inimigo->olhandoParaFrente = movimento.y > 0.0f;
        inimigo->olhandoVertical = true;
    } else {
        inimigo->olhandoEsquerda = movimento.x < 0.0f;
        inimigo->olhandoVertical = false;
    }

    inimigo->tempoAnimacaoAndando += deltaTempo;
    if (inimigo->tempoAnimacaoAndando >= INTERVALO_ANIMACAO_INIMIGO) {
        inimigo->tempoAnimacaoAndando -= INTERVALO_ANIMACAO_INIMIGO;
        inimigo->quadroAnimacaoAndando = !inimigo->quadroAnimacaoAndando;
    }
}

bool PosicionarInimigo(Inimigo *inimigo, Vector2 posicaoJogador){
    *inimigo = (Inimigo){0};

    if (!EncontrarPosicaoAcessivelAleatoria(posicaoJogador, DISTANCIA_MINIMA_NASCIMENTO,
                                            NULL, 0, 0.0f, &inimigo->posicao)) return false;
    EscolherAlvoPatrulha(inimigo);
    return true;
}

void AtualizarInimigo(Inimigo *inimigo, Vector2 posicaoJogador, bool lanternaAcesa, float deltaTempo){
    const float alcance = lanternaAcesa ? ALCANCE_VISAO_LANTERNA_ACESA : ALCANCE_VISAO_ESCURO;
    const bool vendoJogador =
        Vector2DistanceSqr(inimigo->posicao, posicaoJogador) <= alcance * alcance &&
        LinhaLivre(inimigo->posicao, posicaoJogador);

    if (vendoJogador) {
        inimigo->tempoRecalculo -= deltaTempo;
        if (inimigo->estado != INIMIGO_PERSEGUINDO || inimigo->tempoRecalculo <= 0.0f) {
            AtualizarMapaDistancias(posicaoJogador);
            inimigo->tempoRecalculo = INTERVALO_RECALCULO_CAMINHO;
        }
        inimigo->estado = INIMIGO_PERSEGUINDO;
    } else if (inimigo->estado == INIMIGO_PERSEGUINDO) {
        // o mapa ainda aponta para o ultimo lugar em que o jogador foi visto
        inimigo->estado = INIMIGO_PROCURANDO;
        inimigo->tempoProcurando = 0.0f;
    } else if (ChegouAoAlvoDoMapa(inimigo->posicao)) {
        if (inimigo->estado == INIMIGO_PATRULHANDO) {
            EscolherAlvoPatrulha(inimigo);
        } else {
            inimigo->tempoProcurando += deltaTempo;
            if (inimigo->tempoProcurando >= TEMPO_PROCURANDO) EscolherAlvoPatrulha(inimigo);
        }
    }

    const float velocidade = inimigo->estado == INIMIGO_PATRULHANDO
        ? VELOCIDADE_PATRULHA : VELOCIDADE_PERSEGUICAO;
    Andar(inimigo, velocidade * deltaTempo, deltaTempo);
}

bool InimigoPegouJogador(const Inimigo *inimigo, Vector2 posicaoJogador){
    return Vector2DistanceSqr(inimigo->posicao, posicaoJogador) <= RAIO_CAPTURA * RAIO_CAPTURA &&
           LinhaLivre(inimigo->posicao, posicaoJogador);
}
