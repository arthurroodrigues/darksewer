#include "ordem_desenho.h"

// Insertion sort pela base, do menor para o maior valor vertical.
// Quem esta mais acima na tela e desenhado primeiro e fica atras de quem esta mais abaixo.
// De um quadro para o outro a ordem quase nao muda, que e o melhor caso deste algoritmo.
// E estavel: em empate, mantem a ordem em que os elementos entraram.
void OrdenarPorProfundidade(ElementoDesenho *elementos, int quantidade){
    for (int i = 1; i < quantidade; i++) {
        const ElementoDesenho atual = elementos[i];
        int j = i - 1;

        // empurra para a direita todo mundo que esta mais abaixo que o atual
        while (j >= 0 && elementos[j].base > atual.base) {
            elementos[j + 1] = elementos[j];
            j--;
        }
        elementos[j + 1] = atual;
    }
}
