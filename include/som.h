#ifndef SOM_H
#define SOM_H

#include "main.h"
#include <stddef.h>

bool ResolverArquivoAudio(const char *caminhoRelativo, char *bufferCaminho, size_t tamanhoBuffer);
bool InicializarSons(Sons *estadoSons);
void LiberarSons(Sons *estadoSons);
void AtualizarPassos(Sons *estadoSons, bool teclasMovimentoPressionadas);
void TocarSomColeta(Sons *estadoSons);

#endif
