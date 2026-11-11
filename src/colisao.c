#include "colisao.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include "raymath.h"

#define PAREDES_ARQUIVO "assets\\mapa_paredes.png"
#define MEIA_LARGURA_HITBOX_PES 2.5f
#define ALTURA_HITBOX_PES 2.0f
#define DESLOCAMENTO_HITBOX_PES_Y 2.0f
#define PASSO_GRADE_ACESSIBILIDADE 2.0f

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

bool EncontrarPosicaoAcessivelAleatoria(Vector2 origem, float distanciaMinima,
                                        const Vector2 *posicoesOcupadas,
                                        int quantidadePosicoesOcupadas,
                                        float distanciaMinimaEntrePosicoes,
                                        Vector2 *posicao){
    if (pixelsMascaraParedes == NULL || posicao == NULL || distanciaMinima < 0.0f ||
        distanciaMinimaEntrePosicoes < 0.0f || quantidadePosicoesOcupadas < 0 ||
        (quantidadePosicoesOcupadas > 0 && posicoesOcupadas == NULL)) {
        TraceLog(LOG_ERROR, "Nao foi possivel procurar uma posicao acessivel no mapa.");
        return false;
    }

    const int colunas = (int)((float)MAPA_LARGURA / PASSO_GRADE_ACESSIBILIDADE) + 1;
    const int linhas = (int)((float)MAPA_ALTURA / PASSO_GRADE_ACESSIBILIDADE) + 1;
    const int totalPosicoes = colunas * linhas;
    int *fila = malloc((size_t)totalPosicoes * sizeof(*fila));
    unsigned char *visitadas = calloc((size_t)totalPosicoes, sizeof(*visitadas));

    if (fila == NULL || visitadas == NULL) {
        TraceLog(LOG_ERROR, "Falha ao alocar memoria para procurar uma posicao acessivel.");
        free(fila);
        free(visitadas);
        return false;
    }

    const int origemX = (int)roundf(origem.x / PASSO_GRADE_ACESSIBILIDADE);
    const int origemY = (int)roundf(origem.y / PASSO_GRADE_ACESSIBILIDADE);
    if (origemX < 0 || origemX >= colunas || origemY < 0 || origemY >= linhas ||
        !PodeOcupar((Vector2){origemX * PASSO_GRADE_ACESSIBILIDADE,
                              origemY * PASSO_GRADE_ACESSIBILIDADE})) {
        TraceLog(LOG_ERROR, "A origem da busca por posicao acessivel esta bloqueada.");
        free(fila);
        free(visitadas);
        return false;
    }

    const int indiceOrigem = origemY * colunas + origemX;
    int inicioFila = 0;
    int fimFila = 0;
    fila[fimFila++] = indiceOrigem;
    visitadas[indiceOrigem] = 1;

    while (inicioFila < fimFila) {
        const int indice = fila[inicioFila++];
        const int x = indice % colunas;
        const int y = indice / colunas;
        const int vizinhos[4] = {indice - 1, indice + 1, indice - colunas, indice + colunas};

        for (int i = 0; i < 4; i++) {
            if ((i == 0 && x == 0) || (i == 1 && x == colunas - 1) ||
                (i == 2 && y == 0) || (i == 3 && y == linhas - 1)) continue;
            const int vizinho = vizinhos[i];
            const int vizinhoX = vizinho % colunas;
            const int vizinhoY = vizinho / colunas;
            if (vizinhoX < 0 || vizinhoX >= colunas || vizinhoY < 0 ||
                vizinhoY >= linhas || visitadas[vizinho]) continue;

            visitadas[vizinho] = 1;
            const Vector2 candidata = {
                (float)vizinhoX * PASSO_GRADE_ACESSIBILIDADE,
                (float)vizinhoY * PASSO_GRADE_ACESSIBILIDADE
            };
            if (!PodeOcupar(candidata)) continue;
            fila[fimFila++] = vizinho;
        }
    }

    int quantidadeCandidatas = 0;
    const float distanciaMinimaSqr = distanciaMinima * distanciaMinima;
    const float distanciaEntrePosicoesSqr =
        distanciaMinimaEntrePosicoes * distanciaMinimaEntrePosicoes;
    bool encontrouPosicao = false;
    for (int i = 0; i < fimFila; i++) {
        const int indice = fila[i];
        const int x = indice % colunas;
        const int y = indice / colunas;
        const Vector2 candidata = {
            (float)x * PASSO_GRADE_ACESSIBILIDADE,
            (float)y * PASSO_GRADE_ACESSIBILIDADE
        };
        if (candidata.x < 8.0f || candidata.x > MAPA_LARGURA - 8.0f ||
            candidata.y < 8.0f || candidata.y > MAPA_ALTURA - 8.0f ||
            Vector2DistanceSqr(origem, candidata) < distanciaMinimaSqr) continue;

        bool muitoProxima = false;
        for (int j = 0; j < quantidadePosicoesOcupadas; j++) {
            if (Vector2DistanceSqr(posicoesOcupadas[j], candidata) <
                    distanciaEntrePosicoesSqr) {
                muitoProxima = true;
                break;
            }
        }
        if (muitoProxima) continue;

        quantidadeCandidatas++;
        if (GetRandomValue(0, quantidadeCandidatas - 1) == 0) {
            *posicao = candidata;
            encontrouPosicao = true;
        }
    }

    free(fila);
    free(visitadas);

    if (!encontrouPosicao) {
        TraceLog(LOG_ERROR, "Nao ha posicoes acessiveis para o item no mapa.");
    }
    return encontrouPosicao;
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
