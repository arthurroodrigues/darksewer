#ifndef BARRA_ITENS_H
#define BARRA_ITENS_H

#include "raylib.h"

typedef struct loot {
    int numero;
    struct loot *anterior;
    struct loot *proximo;
} loot;

loot *CriarHotbar(void);
void LiberarHotbar(loot *primeiro);
void DesenharHotbar(loot *primeiro, loot *selecionado, int larguraTela,
                    int alturaTela);

#endif
