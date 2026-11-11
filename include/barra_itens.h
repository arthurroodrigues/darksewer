#ifndef BARRA_ITENS_H
#define BARRA_ITENS_H

#include "raylib.h"

typedef enum {
    ITEM_NENHUM,
    ITEM_BATERIA
} ItemHotbar;

#define QUANTIDADE_MAXIMA_POR_SLOT 3

typedef struct loot {
    ItemHotbar item;
    int quantidade;
    struct loot *anterior;
    struct loot *proximo;
} loot;

loot *CriarHotbar(void);
void LiberarHotbar(loot *primeiro);
bool AdicionarItemHotbar(loot *primeiro, ItemHotbar item);
void DesenharHotbar(loot *primeiro, loot *selecionado, Texture2D bateria);

#endif
