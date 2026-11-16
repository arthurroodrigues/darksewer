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

bool RemoverItemHotbar(loot *slot){
    if (slot == NULL || slot->item == ITEM_NENHUM) return false;

    slot->quantidade--;
    if (slot->quantidade <= 0) {
        slot->item = ITEM_NENHUM;
        slot->quantidade = 0;
    }
    return true;
}

void EsvaziarHotbar(loot *primeiro){
    if (primeiro == NULL) return;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        no->item = ITEM_NENHUM;
        no->quantidade = 0;
        no = no->proximo;
    }
}

int ContarItemHotbar(loot *primeiro, ItemHotbar item){
    if (primeiro == NULL) return 0;

    int total = 0;
    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        if (no->item == item) total += no->quantidade;
        no = no->proximo;
    }
    return total;
}

void RemoverTodosDoItemHotbar(loot *primeiro, ItemHotbar item){
    if (primeiro == NULL) return;

    loot *no = primeiro;
    for (int i = 0; i < 5; i++) {
        if (no->item == item) {
            no->item = ITEM_NENHUM;
            no->quantidade = 0;
        }
        no = no->proximo;
    }
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
        DesenharItem(no->item,
                     (Vector2){(float)x + (float)tamanho * 0.5f, (float)y + (float)tamanho * 0.5f},
                     (float)tamanho - 16.0f, bateria);
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

// Chave desenhada com formas: argola a esquerda, haste e dois dentes.
static void DesenharChave(Vector2 centro, float tamanho){
    const Color cor = {212, 175, 55, 255};
    const float raio = tamanho * 0.22f;
    const float espessura = tamanho * 0.12f;
    const Vector2 argola = {centro.x - tamanho * 0.28f, centro.y};

    DrawRing(argola, raio - espessura, raio, 0.0f, 360.0f, 24, cor);
    DrawRectangleRec((Rectangle){argola.x + raio - espessura * 0.5f, centro.y - espessura * 0.5f,
                                 tamanho * 0.56f, espessura}, cor);
    DrawRectangleRec((Rectangle){centro.x + tamanho * 0.22f, centro.y, espessura, tamanho * 0.2f}, cor);
    DrawRectangleRec((Rectangle){centro.x + tamanho * 0.38f, centro.y, espessura, tamanho * 0.16f}, cor);
}

// Desenha um item centrado em um ponto, cabendo em um quadrado de lado `tamanho`.
void DesenharItem(ItemHotbar item, Vector2 centro, float tamanho, Texture2D bateria){
    if (item == ITEM_BATERIA && bateria.id != 0) {
        const int maiorLado = bateria.width > bateria.height ? bateria.width : bateria.height;
        const float escala = tamanho / (float)maiorLado;
        const float largura = (float)bateria.width * escala;
        const float altura = (float)bateria.height * escala;
        const Rectangle origem = {0.0f, 0.0f, (float)bateria.width, (float)bateria.height};
        const Rectangle destino = {centro.x - largura * 0.5f, centro.y - altura * 0.5f, largura, altura};
        DrawTexturePro(bateria, origem, destino, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    } else if (item == ITEM_CHAVE) {
        DesenharChave(centro, tamanho);
    }
}
