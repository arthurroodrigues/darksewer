#ifndef RANKING_H
#define RANKING_H

#include "raylib.h"

#define TAMANHO_NOME_RANKING 10
#define QUANTIDADE_MAXIMA_RANKING 10

typedef struct RegistroRanking {
    char nome[TAMANHO_NOME_RANKING + 1];
    float tempo;
    struct RegistroRanking *proximo;
} RegistroRanking;

RegistroRanking *CarregarRanking(const char *caminho);
RegistroRanking *InserirNoRanking(RegistroRanking **primeiro, const char *nome, float tempo);
bool SalvarRanking(const RegistroRanking *primeiro, const char *caminho);
void LiberarRanking(RegistroRanking *primeiro);
const char *FormatarTempo(float segundos);
void DesenharRanking(const RegistroRanking *primeiro, const RegistroRanking *destaque,
                     int larguraTela, int alturaTela);

#endif
