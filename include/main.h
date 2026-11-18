#ifndef MAIN_H
#define MAIN_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Music musicaFundo;
    Music musicaPassos;
    Music musicaGameplay;
    Sound somColeta;
    bool dispositivoAudioAtivo;
    bool somPassosTocando;
    bool musicaGameplayTocando;
} Sons;

#endif
