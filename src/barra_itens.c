#include "barra_itens.h"
#include <stdlib.h>

loot *CriarHotbar(void){
    loot *primeiro = NULL;
    loot *ultimo = NULL;

    for (int i = 0; i < 5; i++) {
        loot *novo = malloc(sizeof(loot));
        novo->numero = i + 1;
        novo->anterior = ultimo;
        novo->proximo = NULL;

        if (ultimo == NULL) primeiro = novo;
        else ultimo->proximo = novo;
        ultimo = novo;
    }

    ultimo->proximo = primeiro;
    primeiro->anterior = ultimo;
    return primeiro;
}

void LiberarHotbar(loot *primeiro){
    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        loot *proximo = no->proximo;
        free(no);
        no = proximo;
    }
}

void DesenharHotbar(loot *primeiro, loot *selecionado, int larguraTela, int alturaTela){
    const int tamanho = 48;
    const int espaco = 6;
    const int inicioX = (larguraTela - (5 * tamanho + 4 * espaco)) / 2;
    const int y = alturaTela - tamanho - 20;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        const int x = inicioX + i * (tamanho + espaco);
        const bool ativo = (no == selecionado);

        DrawRectangle(x, y, tamanho, tamanho, (Color){10, 16, 15, 200});
        DrawRectangleLinesEx((Rectangle){x, y, tamanho, tamanho}, ativo ? 3 : 2, ativo ? (Color){150, 112, 72, 255} : (Color){74, 88, 82, 255});
        no = no->proximo;
    }
}
