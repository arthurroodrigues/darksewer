#include "barra_itens.h"
#include <stdlib.h>

loot *CriarHotbar(void){
    loot *primeiro = NULL;
    loot *ultimo = NULL;

    for (int i = 0; i < 5; i++) {
        loot *novo = malloc(sizeof(loot));
        if (novo == NULL) {
            while (primeiro != NULL) {
                loot *proximo = primeiro->proximo;
                free(primeiro);
                primeiro = proximo;
            }
            return NULL;
        }
        novo->item = ITEM_NENHUM;
        novo->quantidade = 0;
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

bool AdicionarItemHotbar(loot *primeiro, ItemHotbar item){
    if (primeiro == NULL || item == ITEM_NENHUM) return false;

    loot *no = primeiro;

    for (int i = 0; i < 5; i++) {
        if (no->item == item && no->quantidade < QUANTIDADE_MAXIMA_POR_SLOT) {
            no->quantidade++;
            return true;
        }
        no = no->proximo;
    }

    no = primeiro;
    for (int i = 0; i < 5; i++) {
        if (no->item == ITEM_NENHUM) {
            no->item = item;
            no->quantidade = 1;
            return true;
        }
        no = no->proximo;
    }

    return false;
}

void LiberarHotbar(loot *primeiro){
    if (primeiro == NULL) return;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        loot *proximo = no->proximo;
        free(no);
        no = proximo;
    }
}

void DesenharHotbar(loot *primeiro, loot *selecionado, Texture2D bateria){
    const int tamanho = 60;
    const int espaco = 6;
    const int inicioX = 20;
    const int y = 20;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        const int x = inicioX + i * (tamanho + espaco);
        const bool ativo = (no == selecionado);

        DrawRectangle(x, y, tamanho, tamanho, (Color){10, 16, 15, 200});
        if (no->item == ITEM_BATERIA && bateria.id != 0) {
            const float areaDisponivel = (float)tamanho - 8.0f;
            const float escalaLargura = areaDisponivel / (float)bateria.width;
            const float escalaAltura = areaDisponivel / (float)bateria.height;
            const float escala = escalaLargura < escalaAltura ? escalaLargura : escalaAltura;
            const float largura = (float)bateria.width * escala;
            const float altura = (float)bateria.height * escala;
            const Rectangle origem = {0.0f, 0.0f, (float)bateria.width, (float)bateria.height};
            const Rectangle destino = {
                (float)x + ((float)tamanho - largura) * 0.5f,
                (float)y + ((float)tamanho - altura) * 0.5f,
                largura,
                altura
            };
            DrawTexturePro(bateria, origem, destino, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }
        if (no->quantidade > 1) {
            const char *quantidade = TextFormat("%d", no->quantidade);
            const int tamanhoTexto = MeasureText(quantidade, 14);
            DrawText(quantidade, x + (tamanho - tamanhoTexto) / 2,
                     y + tamanho + 2, 14, WHITE);
        }
        DrawRectangleLinesEx((Rectangle){x, y, tamanho, tamanho}, ativo ? 3 : 2, ativo ? (Color){150, 112, 72, 255} : (Color){74, 88, 82, 255});
        no = no->proximo;
    }
}
