#ifndef JOGADOR_H
#define JOGADOR_H

#include "raylib.h"

void DesenharPersonagem(Vector2 posicao, Texture2D sprite, float alturaNoMapa, bool espelhado);
void DesenharJogador(Vector2 posicaoJogador, Texture2D spriteJogador);
void DesenharVidas(int vidas, int vidasMaximas, int larguraTela);

#endif
