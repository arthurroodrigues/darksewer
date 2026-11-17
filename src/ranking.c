#include "ranking.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TEMPO_MAXIMO_RANKING 359999.0f

RegistroRanking *InserirNoRanking(RegistroRanking **primeiro, const char *nome, float tempo){
    RegistroRanking *novo = malloc(sizeof(*novo));
    if (novo == NULL) return NULL;

    snprintf(novo->nome, sizeof(novo->nome), "%s", nome);
    novo->tempo = tempo;

    RegistroRanking **elo = primeiro;
    int posicao = 0;
    while (*elo != NULL && (*elo)->tempo <= tempo) {
        elo = &(*elo)->proximo;
        posicao++;
    }

    if (posicao >= QUANTIDADE_MAXIMA_RANKING) {
        free(novo);
        return NULL;
    }

    novo->proximo = *elo;
    *elo = novo;

    RegistroRanking *ultimo = *primeiro;
    for (int i = 1; i < QUANTIDADE_MAXIMA_RANKING && ultimo != NULL; i++) ultimo = ultimo->proximo;
    if (ultimo != NULL) {
        LiberarRanking(ultimo->proximo);
        ultimo->proximo = NULL;
    }
    return novo;
}

RegistroRanking *CarregarRanking(const char *caminho){
    RegistroRanking *primeiro = NULL;
    FILE *arquivo = fopen(caminho, "r");
    if (arquivo == NULL) return NULL;

    char linha[128];
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char nome[TAMANHO_NOME_RANKING + 1];
        float tempo;

        if (sscanf(linha, "%10[^;\n];%f", nome, &tempo) != 2) continue;
        if (!isfinite(tempo) || tempo <= 0.0f || tempo > TEMPO_MAXIMO_RANKING) continue;
        InserirNoRanking(&primeiro, nome, tempo);
    }

    fclose(arquivo);
    return primeiro;
}

bool SalvarRanking(const RegistroRanking *primeiro, const char *caminho){
    FILE *arquivo = fopen(caminho, "w");
    if (arquivo == NULL) return false;

    bool gravou = true;
    for (const RegistroRanking *no = primeiro; no != NULL; no = no->proximo) {
        if (fprintf(arquivo, "%s;%.2f\n", no->nome, no->tempo) < 0) gravou = false;
    }
    if (fclose(arquivo) != 0) gravou = false;
    return gravou;
}

void LiberarRanking(RegistroRanking *primeiro){
    while (primeiro != NULL) {
        RegistroRanking *proximo = primeiro->proximo;
        free(primeiro);
        primeiro = proximo;
    }
}

const char *FormatarTempo(float segundos){
    const int decimos = (int)(segundos * 10.0f);
    return TextFormat("%02d:%02d.%d", decimos / 600, (decimos / 10) % 60, decimos % 10);
}

void DesenharRanking(const RegistroRanking *primeiro, const RegistroRanking *destaque,
                     int larguraTela, int alturaTela){
    const char *titulo = "RANKING";
    const char *rodape = "ENTER - Jogar    M - Voltar ao menu";
    const int tamanhoTitulo = 44;
    const int tamanhoLinha = 22;
    const int alturaLinha = 30;
    const int colunaNome = larguraTela / 2 - 170;
    const int colunaTempo = larguraTela / 2 + 170;
    const Color corNormal = {214, 204, 190, 255};
    const Color corDestaque = {212, 175, 55, 255};
    int y = alturaTela / 2 - 230;

    DrawRectangle(0, 0, larguraTela, alturaTela, (Color){0, 0, 0, 200});
    DrawText(titulo, (larguraTela - MeasureText(titulo, tamanhoTitulo)) / 2, y, tamanhoTitulo, corNormal);
    y += tamanhoTitulo + 30;

    if (primeiro == NULL) {
        const char *vazio = "Nenhuma fuga registrada";
        DrawText(vazio, (larguraTela - MeasureText(vazio, tamanhoLinha)) / 2, y, tamanhoLinha, (Color){150, 112, 72, 255});
        y += alturaLinha;
    }

    int posicao = 1;
    for (const RegistroRanking *no = primeiro; no != NULL; no = no->proximo, posicao++) {
        const Color cor = no == destaque ? corDestaque : corNormal;
        const char *tempo = FormatarTempo(no->tempo);

        DrawText(TextFormat("%2d. %s", posicao, no->nome), colunaNome, y, tamanhoLinha, cor);
        DrawText(tempo, colunaTempo - MeasureText(tempo, tamanhoLinha), y, tamanhoLinha, cor);
        y += alturaLinha;
    }

    y += 24;
    DrawText(rodape, (larguraTela - MeasureText(rodape, 20)) / 2, y, 20, (Color){150, 112, 72, 255});
}
