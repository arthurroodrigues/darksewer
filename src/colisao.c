#include "colisao.h"
#include <math.h>
#include <stddef.h>
#include "raymath.h"

#define PAREDES_ARQUIVO "assets\\mapa_paredes.png"
#define MEIA_LARGURA_HITBOX_PES 2.5f
#define ALTURA_HITBOX_PES 2.0f
#define DESLOCAMENTO_HITBOX_PES_Y 2.0f

static Color *pixelsMascaraParedes;
static int larguraMascaraParedes;
static int alturaMascaraParedes;

bool CarregarMascaraParedes(int larguraMapaFonte, int alturaMapaFonte){
    Image imagem = LoadImage(PAREDES_ARQUIVO);

    if (imagem.data == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar a mascara: %s", PAREDES_ARQUIVO);
        return false;
    }

    if (imagem.width != larguraMapaFonte ||
        (imagem.height != alturaMapaFonte && imagem.height != alturaMapaFonte + 1)) {
        TraceLog(LOG_ERROR, "Tamanho da mascara invalido: %d x %d; esperado %d x %d (ou uma linha extra).", imagem.width, imagem.height, larguraMapaFonte, alturaMapaFonte);
        UnloadImage(imagem);
        return false;
    }

    Color *pixels = LoadImageColors(imagem);
    UnloadImage(imagem);
    if (pixels == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar os pixels da mascara de paredes.");
        return false;
    }

    for (int y = 0; y < alturaMapaFonte; y++) {
        for (int x = 0; x < larguraMapaFonte; x++) {
            const int indice = y * larguraMapaFonte + x;
            const Color pixel = pixels[indice];
            const unsigned int brilho =
                (299u * pixel.r + 587u * pixel.g + 114u * pixel.b) / 1000u;
            pixels[indice] = brilho >= 128u ? BLACK : WHITE;
        }
    }

    pixelsMascaraParedes = pixels;
    larguraMascaraParedes = larguraMapaFonte;
    alturaMascaraParedes = alturaMapaFonte;
    return true;
}

void LiberarMascaraParedes(void){
    if (pixelsMascaraParedes == NULL) return;
    UnloadImageColors(pixelsMascaraParedes);
    pixelsMascaraParedes = NULL;
}

static bool PixelParede(int x, int y){
    if (pixelsMascaraParedes == NULL || x < 0 || x >= larguraMascaraParedes ||
        y < 0 || y >= alturaMascaraParedes) return true;

    return pixelsMascaraParedes[y * larguraMascaraParedes + x].r < 128;
}

bool PodeOcupar(Vector2 posicaoJogador){
    const float esquerda = posicaoJogador.x - MEIA_LARGURA_HITBOX_PES;
    const float direita = posicaoJogador.x + MEIA_LARGURA_HITBOX_PES;
    const float topo =
        posicaoJogador.y - DESLOCAMENTO_HITBOX_PES_Y - ALTURA_HITBOX_PES;
    const float base = posicaoJogador.y - DESLOCAMENTO_HITBOX_PES_Y;

    if (esquerda < 0.0f || topo < 0.0f ||
        direita > MAPA_LARGURA || base > MAPA_ALTURA) return false;

    const float escalaMascaraX = (float)larguraMascaraParedes / (float)MAPA_LARGURA;
    const float escalaMascaraY = (float)alturaMascaraParedes / (float)MAPA_ALTURA;
    const int pixelXInicial = (int)floorf(esquerda * escalaMascaraX);
    const int pixelXFinal = (int)ceilf(direita * escalaMascaraX) - 1;
    const int pixelYInicial = (int)floorf(topo * escalaMascaraY);
    const int pixelYFinal = (int)ceilf(base * escalaMascaraY) - 1;

    for (int y = pixelYInicial; y <= pixelYFinal; y++) {
        const float pixelTopo = (float)y / escalaMascaraY;
        const float pixelBase = (float)(y + 1) / escalaMascaraY;
        if (pixelTopo >= base || pixelBase <= topo) continue;

        for (int x = pixelXInicial; x <= pixelXFinal; x++) {
            const float pixelEsquerda = (float)x / escalaMascaraX;
            const float pixelDireita = (float)(x + 1) / escalaMascaraX;
            if (pixelEsquerda >= direita || pixelDireita <= esquerda) continue;
            if (PixelParede(x, y)) return false;
        }
    }
    return true;
}

Vector2 MoverComColisao(Vector2 posicaoJogador, Vector2 deslocamentoJogador){
    const float maiorDistanciaMovida = fmaxf(fabsf(deslocamentoJogador.x), fabsf(deslocamentoJogador.y));
    const float passoMaximo = 0.5f * fminf(
        (float)MAPA_LARGURA / (float)larguraMascaraParedes,
        (float)MAPA_ALTURA / (float)alturaMascaraParedes);
    const int quantidadePassos = (int)ceilf(maiorDistanciaMovida / passoMaximo);
    if (quantidadePassos <= 0) return posicaoJogador;

    const Vector2 deslocamentoPorPasso = Vector2Scale(deslocamentoJogador, 1.0f / (float)quantidadePassos);
    for (int i = 0; i < quantidadePassos; i++) {
        Vector2 posicaoTentativaX = {
            posicaoJogador.x + deslocamentoPorPasso.x, posicaoJogador.y
        };

        if (PodeOcupar(posicaoTentativaX)) posicaoJogador.x = posicaoTentativaX.x;

        Vector2 posicaoTentativaY = {
            posicaoJogador.x, posicaoJogador.y + deslocamentoPorPasso.y
        };
        if (PodeOcupar(posicaoTentativaY)) posicaoJogador.y = posicaoTentativaY.y;
    }
    return posicaoJogador;
}
