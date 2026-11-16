#ifndef BARRA_ITENS_H
#define BARRA_ITENS_H

#include "raylib.h"

typedef enum {
    ITEM_NENHUM,
    ITEM_BATERIA,
    ITEM_CHAVE,
    ITEM_ISCA
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
bool RemoverItemHotbar(loot *slot);
void EsvaziarHotbar(loot *primeiro);
int ContarItemHotbar(loot *primeiro, ItemHotbar item);
void RemoverTodosDoItemHotbar(loot *primeiro, ItemHotbar item);
void DesenharItem(ItemHotbar item, Vector2 centro, float tamanho, Texture2D bateria);
void DesenharHotbar(loot *primeiro, loot *selecionado, Texture2D bateria);

#endif
