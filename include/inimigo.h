#ifndef INIMIGO_H
#define INIMIGO_H

#include "raylib.h"

typedef enum {
    INIMIGO_PATRULHANDO,
    INIMIGO_PERSEGUINDO,
    INIMIGO_PROCURANDO
} EstadoInimigo;

typedef struct {
    Vector2 posicao;
    EstadoInimigo estado;
    float tempoRecalculo;
    float tempoProcurando;
    float tempoAnimacaoAndando;
    bool olhandoEsquerda;
    bool olhandoParaFrente;
    bool olhandoVertical;
    bool quadroAnimacaoAndando;
    bool jogadorEstavaEscondido;
} Inimigo;

bool CarregarSpritesInimigo(void);
void LiberarSpritesInimigo(void);
void DesenharInimigo(const Inimigo *inimigo);
bool PosicionarInimigo(Inimigo *inimigo, Vector2 posicaoJogador);
void AtualizarInimigo(Inimigo *inimigo, Vector2 posicaoJogador, bool lanternaAcesa,
                      bool jogadorEscondido, float deltaTempo);
bool InimigoPegouJogador(const Inimigo *inimigo, Vector2 posicaoJogador);

#endif
