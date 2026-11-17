#ifndef ORDEM_DESENHO_H
#define ORDEM_DESENHO_H

typedef enum {
    DESENHO_ITEM,
    DESENHO_INIMIGO,
    DESENHO_JOGADOR
} TipoDesenho;

typedef struct {
    float base;
    TipoDesenho tipo;
    int indice;
} ElementoDesenho;

void OrdenarPorProfundidade(ElementoDesenho *elementos, int quantidade);

#endif
