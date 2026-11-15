#include "jogador.h"

#define ALTURA_SPRITE_JOGADOR 25.5f

void DesenharPersonagem(Vector2 posicao, Texture2D sprite, float alturaNoMapa, bool espelhado){
    const float escala = alturaNoMapa / (float)sprite.height;
    const float largura = (float)sprite.width * escala;
    const float altura = (float)sprite.height * escala;
    const Rectangle origem = {
        0.0f, 0.0f,
        espelhado ? -(float)sprite.width : (float)sprite.width, (float)sprite.height
    };
    const Rectangle destino = {posicao.x, posicao.y, largura, altura};
    const Vector2 ancora = {largura * 0.5f, altura};
    DrawTexturePro(sprite, origem, destino, ancora, 0.0f, WHITE);
}

void DesenharJogador(Vector2 posicaoJogador, Texture2D spriteJogador){
    DesenharPersonagem(posicaoJogador, spriteJogador, ALTURA_SPRITE_JOGADOR, false);
}
