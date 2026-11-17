#include "ordem_desenho.h"

void OrdenarPorProfundidade(ElementoDesenho *elementos, int quantidade){
    for (int i = 1; i < quantidade; i++) {
        const ElementoDesenho atual = elementos[i];
        int j = i - 1;

        while (j >= 0 && elementos[j].base > atual.base) {
            elementos[j + 1] = elementos[j];
            j--;
        }
        elementos[j + 1] = atual;
    }
}
