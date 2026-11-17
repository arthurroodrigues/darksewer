#ifndef ESCONDERIJO_H
#define ESCONDERIJO_H

#include "raylib.h"

#define QUANTIDADE_ARMARIOS 4
#define QUANTIDADE_CAIXAS 5
#define QUANTIDADE_ESCONDERIJOS (QUANTIDADE_ARMARIOS + QUANTIDADE_CAIXAS)

typedef enum {
    ESCONDERIJO_CAIXA,
    ESCONDERIJO_ARMARIO
} TipoEsconderijo;

typedef struct {
    Vector2 posicao;
    TipoEsconderijo tipo;
} Esconderijo;

bool CarregarTexturasEsconderijo(void);
void LiberarTexturasEsconderijo(void);
void DesenharEsconderijo(const Esconderijo *esconderijo);
int EsconderijoPertoDoJogador(const Esconderijo *esconderijos, int quantidade, Vector2 posicaoJogador);

#endif
