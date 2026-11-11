#ifndef MAIN_H
#define MAIN_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Music musicaFundo;
    Music musicaPassos;
    Sound somColeta;
    bool dispositivoAudioAtivo;
    bool somPassosTocando;
} Sons;

#endif