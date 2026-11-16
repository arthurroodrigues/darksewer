#ifndef ORDEM_DESENHO_H
#define ORDEM_DESENHO_H

typedef enum {
    DESENHO_ITEM,
    DESENHO_ISCA,
    DESENHO_INIMIGO,
    DESENHO_JOGADOR
} TipoDesenho;

// Um elemento do chao a desenhar. A base e a posicao vertical dos pes do
// personagem ou da borda de baixo do item; o indice diz qual item do mapa.
typedef struct {
    float base;
    TipoDesenho tipo;
    int indice;
} ElementoDesenho;

void OrdenarPorProfundidade(ElementoDesenho *elementos, int quantidade);

#endif
