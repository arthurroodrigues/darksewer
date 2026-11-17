#ifndef MENU_H
#define MENU_H

#include "raylib.h"

Rectangle CalcularDestinoTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela);
Rectangle CalcularBotaoTelaInicial(Texture2D imagemFundo, Rectangle destinoImagemFundo);
void DesenharTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela, float opacidade);
void DesenharCaixaDialogo(const char *texto, const char *textoCompleto,
                          bool ultimaFala, bool falaCompleta, int larguraTela, int alturaTela);
void DesenharTelaPausa(int larguraTela, int alturaTela);
void DesenharTelaFim(int larguraTela, int alturaTela);
void DesenharTelaVitoria(int larguraTela, int alturaTela, float tempo, const char *nome);

#endif
