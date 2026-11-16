#ifndef INIMIGO_H
#define INIMIGO_H

#include "raylib.h"

typedef enum {
    INIMIGO_PATRULHANDO,
    INIMIGO_PERSEGUINDO,
    INIMIGO_PROCURANDO,
    INIMIGO_DISTRAIDO
} EstadoInimigo;

typedef struct {
    Vector2 posicao;
    EstadoInimigo estado;
    float tempoRecalculo;
    float tempoProcurando;
    float tempoDistraido;
    float tempoAnimacaoAndando;
    bool olhandoEsquerda;
    bool olhandoParaFrente;
    bool olhandoVertical;
    bool quadroAnimacaoAndando;
} Inimigo;

bool CarregarSpritesInimigo(void);
void LiberarSpritesInimigo(void);
void DesenharInimigo(const Inimigo *inimigo);
bool PosicionarInimigo(Inimigo *inimigo, Vector2 posicaoJogador);
void AtualizarInimigo(Inimigo *inimigo, Vector2 posicaoJogador, bool lanternaAcesa, float deltaTempo);
bool InimigoPegouJogador(const Inimigo *inimigo, Vector2 posicaoJogador);
void AtrairInimigo(Inimigo *inimigo, Vector2 ponto, float duracao);

#endif
