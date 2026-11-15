#include "inimigo.h"
#include "colisao.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

#define VELOCIDADE_PATRULHA 45.0f
#define VELOCIDADE_PERSEGUICAO 100.0f
#define ALCANCE_VISAO_LANTERNA_ACESA 110.0f
#define ALCANCE_VISAO_ESCURO 35.0f
#define RAIO_CAPTURA 8.0f
#define INTERVALO_RECALCULO_CAMINHO 0.4f
#define TEMPO_PROCURANDO 3.0f
#define DISTANCIA_MINIMA_NASCIMENTO 250.0f
#define DISTANCIA_MINIMA_PATRULHA 80.0f
#define INTERVALO_ANIMACAO_INIMIGO 0.18f

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

    if (intensidadeVertical > intensidadeHorizontal ||
        (intensidadeVertical == intensidadeHorizontal && inimigo->olhandoVertical)) {
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
