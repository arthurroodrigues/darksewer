#ifndef COLISAO_H
#define COLISAO_H

#include "raylib.h"

#define MAPA_LARGURA 552
#define MAPA_ALTURA 712

bool CarregarMascaraParedes(void);
void LiberarMascaraParedes(void);
bool PodeOcupar(Vector2 posicaoJogador);
bool EncontrarPosicaoAcessivelAleatoria(Vector2 origem, float distanciaMinima,
                                        const Vector2 *posicoesOcupadas,
                                        int quantidadePosicoesOcupadas,
                                        float distanciaMinimaEntrePosicoes,
                                        Vector2 *posicao);
Vector2 MoverComColisao(Vector2 posicaoJogador, Vector2 deslocamentoJogador);
bool AtualizarMapaDistancias(Vector2 alvo);
Vector2 ProximoPontoNoCaminho(Vector2 posicao);
bool ChegouAoAlvoDoMapa(Vector2 posicao);
bool LinhaLivre(Vector2 origem, Vector2 destino);
void DefinirZonaBloqueada(Rectangle zona);

#endif
