#ifndef LANTERNA_H
#define LANTERNA_H

#include "raylib.h"

typedef struct {
    float carga;
    bool ligada;
} Lanterna;

Lanterna CriarLanterna(void);
void AlternarLanterna(Lanterna *lanterna);
void AtualizarLanterna(Lanterna *lanterna, float deltaTempo);
bool RecarregarLanterna(Lanterna *lanterna);
float RaioLanterna(const Lanterna *lanterna);
bool LanternaAcesa(const Lanterna *lanterna);
void DesenharCargaLanterna(const Lanterna *lanterna, int larguraTela);

#endif
