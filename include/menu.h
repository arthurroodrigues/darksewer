#ifndef MENU_H
#define MENU_H

#include "raylib.h"

Rectangle CalcularDestinoTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela);
Rectangle CalcularBotaoTelaInicial(Texture2D imagemFundo, Rectangle destinoImagemFundo);
void DesenharTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela, float opacidade);
void DesenharTelaPausa(int larguraTela, int alturaTela);
void DesenharTelaFim(int larguraTela, int alturaTela);
void DesenharTelaVitoria(int larguraTela, int alturaTela);

#endif
