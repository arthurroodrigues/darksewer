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
static unsigned char *gradeLivre;
static int *distanciasGrade;
static int *filaGrade;
static int colunasGrade;
static int linhasGrade;

static Vector2 CentroCelula(int indice){
    return (Vector2){
        (float)(indice % colunasGrade) * PASSO_GRADE_ACESSIBILIDADE,
        (float)(indice / colunasGrade) * PASSO_GRADE_ACESSIBILIDADE
    };
}

static bool CriarGradeCaminhada(void){
    colunasGrade = (int)((float)MAPA_LARGURA / PASSO_GRADE_ACESSIBILIDADE) + 1;
    linhasGrade = (int)((float)MAPA_ALTURA / PASSO_GRADE_ACESSIBILIDADE) + 1;
    const int totalCelulas = colunasGrade * linhasGrade;
    gradeLivre = malloc((size_t)totalCelulas * sizeof(*gradeLivre));
    distanciasGrade = malloc((size_t)totalCelulas * sizeof(*distanciasGrade));
    filaGrade = malloc((size_t)totalCelulas * sizeof(*filaGrade));

    if (gradeLivre == NULL || distanciasGrade == NULL || filaGrade == NULL) {
        TraceLog(LOG_ERROR, "Falha ao alocar memoria para a grade de caminhada.");
        LiberarMascaraParedes();
        return false;
    }

    for (int i = 0; i < totalCelulas; i++) {
        gradeLivre[i] = PodeOcupar(CentroCelula(i));
        distanciasGrade[i] = -1;
    }
    return true;
}

static int CelulaLivreMaisProxima(Vector2 posicao){
    if (gradeLivre == NULL) return -1;

    const int x = (int)roundf(posicao.x / PASSO_GRADE_ACESSIBILIDADE);
    const int y = (int)roundf(posicao.y / PASSO_GRADE_ACESSIBILIDADE);
    int melhor = -1;
    float melhorDistancia = 0.0f;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const int vizinhoX = x + dx;
            const int vizinhoY = y + dy;
            if (vizinhoX < 0 || vizinhoX >= colunasGrade ||
                vizinhoY < 0 || vizinhoY >= linhasGrade) continue;

            const int indice = vizinhoY * colunasGrade + vizinhoX;
            if (!gradeLivre[indice]) continue;

            const float distancia = Vector2DistanceSqr(posicao, CentroCelula(indice));
            if (melhor < 0 || distancia < melhorDistancia) {
                melhor = indice;
                melhorDistancia = distancia;
            }
        }
    }
    return melhor;
}

bool CarregarMascaraParedes(void){
    Image imagem = LoadImage(PAREDES_ARQUIVO);

    if (imagem.data == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar a mascara: %s", PAREDES_ARQUIVO);
        return false;
    }

    if (imagem.width <= 0 || imagem.height <= 0) {
        TraceLog(LOG_ERROR, "A mascara de paredes tem dimensoes invalidas: %d x %d.", imagem.width, imagem.height);
        UnloadImage(imagem);
        return false;
    }

    const int larguraMascara = imagem.width;
    const int alturaMascara = imagem.height;
    Color *pixels = LoadImageColors(imagem);
    UnloadImage(imagem);
    if (pixels == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar os pixels da mascara de paredes.");
        return false;
    }

    for (int y = 0; y < alturaMascara; y++) {
        for (int x = 0; x < larguraMascara; x++) {
            const int indice = y * larguraMascara + x;
            const Color pixel = pixels[indice];
            const unsigned int brilho =
                (299u * pixel.r + 587u * pixel.g + 114u * pixel.b) / 1000u;
            pixels[indice] = brilho >= 128u ? WHITE : BLACK;
        }
    }

    pixelsMascaraParedes = pixels;
    larguraMascaraParedes = larguraMascara;
    alturaMascaraParedes = alturaMascara;
    return CriarGradeCaminhada();
}

void LiberarMascaraParedes(void){
    free(gradeLivre);
    free(distanciasGrade);
    free(filaGrade);
    gradeLivre = NULL;
    distanciasGrade = NULL;
    filaGrade = NULL;

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

    const int indiceOrigem = CelulaLivreMaisProxima(origem);
    if (indiceOrigem < 0) {
        TraceLog(LOG_ERROR, "A origem da busca por posicao acessivel esta bloqueada.");
        free(fila);
        free(visitadas);
        return false;
    }

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
            if (!gradeLivre[vizinho]) continue;
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

// limitacao: existe um unico mapa de distancias, entao so serve a um inimigo por vez.
bool AtualizarMapaDistancias(Vector2 alvo){
    const int origem = CelulaLivreMaisProxima(alvo);
    if (origem < 0) return false;

    const int totalCelulas = colunasGrade * linhasGrade;
    for (int i = 0; i < totalCelulas; i++) distanciasGrade[i] = -1;

    int inicioFila = 0;
    int fimFila = 0;
    filaGrade[fimFila++] = origem;
    distanciasGrade[origem] = 0;

    while (inicioFila < fimFila) {
        const int indice = filaGrade[inicioFila++];
        const int x = indice % colunasGrade;
        const int y = indice / colunasGrade;
        const int vizinhos[4] = {indice - 1, indice + 1, indice - colunasGrade, indice + colunasGrade};
        const bool dentroDaGrade[4] = {x > 0, x < colunasGrade - 1, y > 0, y < linhasGrade - 1};

        for (int i = 0; i < 4; i++) {
            if (!dentroDaGrade[i] || !gradeLivre[vizinhos[i]] ||
                distanciasGrade[vizinhos[i]] >= 0) continue;
            distanciasGrade[vizinhos[i]] = distanciasGrade[indice] + 1;
            filaGrade[fimFila++] = vizinhos[i];
        }
    }
    return true;
}

Vector2 ProximoPontoNoCaminho(Vector2 posicao){
    const int celula = CelulaLivreMaisProxima(posicao);
    if (celula < 0 || distanciasGrade[celula] < 0) return posicao;
    if (distanciasGrade[celula] == 0) return CentroCelula(celula);

    const int x = celula % colunasGrade;
    const int y = celula / colunasGrade;
    int melhor = celula;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            const int vizinhoX = x + dx;
            const int vizinhoY = y + dy;
            if (vizinhoX < 0 || vizinhoX >= colunasGrade ||
                vizinhoY < 0 || vizinhoY >= linhasGrade) continue;

            const int vizinho = vizinhoY * colunasGrade + vizinhoX;
            if (distanciasGrade[vizinho] < 0 ||
                distanciasGrade[vizinho] >= distanciasGrade[melhor]) continue;

            // diagonal so vale quando nao corta a quina de uma parede
            if (dx != 0 && dy != 0 &&
                (!gradeLivre[y * colunasGrade + vizinhoX] ||
                 !gradeLivre[vizinhoY * colunasGrade + x])) continue;
            melhor = vizinho;
        }
    }
    return CentroCelula(melhor);
}

bool ChegouAoAlvoDoMapa(Vector2 posicao){
    const int celula = CelulaLivreMaisProxima(posicao);
    return celula >= 0 && distanciasGrade[celula] == 0;
}

bool LinhaLivre(Vector2 origem, Vector2 destino){
    const float escalaMascaraX = (float)larguraMascaraParedes / (float)MAPA_LARGURA;
    const float escalaMascaraY = (float)alturaMascaraParedes / (float)MAPA_ALTURA;
    const float alturaPes = DESLOCAMENTO_HITBOX_PES_Y + ALTURA_HITBOX_PES * 0.5f;
    const int amostras = (int)ceilf(Vector2Distance(origem, destino) * 2.0f) + 1;

    for (int i = 0; i <= amostras; i++) {
        const Vector2 ponto = Vector2Lerp(origem, destino, (float)i / (float)amostras);
        if (PixelParede((int)(ponto.x * escalaMascaraX),
                        (int)((ponto.y - alturaPes) * escalaMascaraY))) return false;
    }
    return true;
}
