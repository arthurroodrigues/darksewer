#ifndef COLISAO_H
#define COLISAO_H

#include "raylib.h"

#define MAPA_LARGURA 768
#define MAPA_ALTURA 991

bool CarregarMascaraParedes(int larguraMapaFonte, int alturaMapaFonte);
void LiberarMascaraParedes(void);
bool PodeOcupar(Vector2 posicaoJogador);
Vector2 MoverComColisao(Vector2 posicaoJogador, Vector2 deslocamentoJogador);

#endif
