#include "esconderijo.h"
#include "jogador.h"
#include "raymath.h"

#define CAIXA_ARQUIVO "assets/caixa.png"
#define ARMARIO_ARQUIVO "assets/guarda_roupa.png"
#define ALTURA_CAIXA 11.0f
#define ALTURA_ARMARIO 25.0f
#define RAIO_INTERACAO_ESCONDERIJO 13.0f

static Texture2D texturaCaixa;
static Texture2D texturaArmario;

bool CarregarTexturasEsconderijo(void){
    texturaCaixa = LoadTexture(CAIXA_ARQUIVO);
    texturaArmario = LoadTexture(ARMARIO_ARQUIVO);

    if (texturaCaixa.id == 0 || texturaArmario.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar caixa.png ou guarda_roupa.png.");
        LiberarTexturasEsconderijo();
        return false;
    }
    SetTextureFilter(texturaCaixa, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(texturaArmario, TEXTURE_FILTER_BILINEAR);
    return true;
}

void LiberarTexturasEsconderijo(void){
    if (texturaCaixa.id != 0) UnloadTexture(texturaCaixa);
    if (texturaArmario.id != 0) UnloadTexture(texturaArmario);
    texturaCaixa = (Texture2D){0};
    texturaArmario = (Texture2D){0};
}

void DesenharEsconderijo(const Esconderijo *esconderijo){
    if (esconderijo->tipo == ESCONDERIJO_ARMARIO) {
        DesenharPersonagem(esconderijo->posicao, texturaArmario, ALTURA_ARMARIO, false);
    } else {
        DesenharPersonagem(esconderijo->posicao, texturaCaixa, ALTURA_CAIXA, false);
    }
}

int EsconderijoPertoDoJogador(const Esconderijo *esconderijos, int quantidade, Vector2 posicaoJogador){
    int melhor = -1;
    float menorDistancia = RAIO_INTERACAO_ESCONDERIJO * RAIO_INTERACAO_ESCONDERIJO;

    for (int i = 0; i < quantidade; i++) {
        const float distancia = Vector2DistanceSqr(posicaoJogador, esconderijos[i].posicao);
        if (distancia <= menorDistancia) {
            menorDistancia = distancia;
            melhor = i;
        }
    }
    return melhor;
}
