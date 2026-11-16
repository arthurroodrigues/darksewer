#include "lanterna.h"
#include <math.h>

#define DURACAO_BATERIA_SEGUNDOS 90.0f
#define RAIO_LANTERNA_MAXIMO 70.0f
#define RAIO_LANTERNA_MINIMO 18.0f
#define CARGA_INICIO_ENFRAQUECIMENTO 0.25f
#define CARGA_MAXIMA_PARA_RECARREGAR 0.9f

Lanterna CriarLanterna(void){
    return (Lanterna){.carga = 1.0f, .ligada = true};
}

void AlternarLanterna(Lanterna *lanterna){
    lanterna->ligada = !lanterna->ligada;
}

void AtualizarLanterna(Lanterna *lanterna, float deltaTempo){
    if (!lanterna->ligada) return;

    lanterna->carga = fmaxf(lanterna->carga - deltaTempo / DURACAO_BATERIA_SEGUNDOS, 0.0f);
}

bool RecarregarLanterna(Lanterna *lanterna){
    if (lanterna->carga > CARGA_MAXIMA_PARA_RECARREGAR) return false;

    lanterna->carga = 1.0f;
    return true;
}

float RaioLanterna(const Lanterna *lanterna){
    if (!lanterna->ligada) return RAIO_LANTERNA_MINIMO;

    const float intensidade = fminf(lanterna->carga / CARGA_INICIO_ENFRAQUECIMENTO, 1.0f);
    return RAIO_LANTERNA_MINIMO + (RAIO_LANTERNA_MAXIMO - RAIO_LANTERNA_MINIMO) * intensidade;
}

bool LanternaAcesa(const Lanterna *lanterna){
    return lanterna->ligada && lanterna->carga > 0.0f;
}

void DesenharCargaLanterna(const Lanterna *lanterna, int larguraTela){
    const int largura = 120;
    const int altura = 14;
    const int x = larguraTela - largura - 20;
    const int y = 20;
    const Color corCarga = !lanterna->ligada
        ? (Color){74, 88, 82, 255}
        : (lanterna->carga <= CARGA_INICIO_ENFRAQUECIMENTO
            ? (Color){170, 52, 40, 255} : (Color){150, 112, 72, 255});

    DrawRectangle(x, y, largura, altura, (Color){10, 16, 15, 200});
    DrawRectangle(x, y, (int)((float)largura * lanterna->carga), altura, corCarga);
    DrawRectangleLinesEx((Rectangle){x, y, largura, altura}, 2, (Color){74, 88, 82, 255});
}
