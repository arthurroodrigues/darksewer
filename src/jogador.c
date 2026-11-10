#include "jogador.h"

#define ALTURA_SPRITE_JOGADOR 32.5f

void DesenharJogador(Vector2 posicaoJogador, Texture2D spriteJogador){
    const float escala = ALTURA_SPRITE_JOGADOR / (float)spriteJogador.height;
    const float largura = (float)spriteJogador.width * escala;
    const float altura = (float)spriteJogador.height * escala;
    const Rectangle origem = {
        0.0f, 0.0f, (float)spriteJogador.width, (float)spriteJogador.height
    };
    const Rectangle destino = {posicaoJogador.x, posicaoJogador.y, largura, altura};
    const Vector2 ancora = {largura * 0.5f, altura};
    DrawTexturePro(spriteJogador, origem, destino, ancora, 0.0f, WHITE);
}
