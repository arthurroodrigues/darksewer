#include "menu.h"
#include "ranking.h"
#include <math.h>
#include <string.h>

static int DesenharTextoQuebrado(const char *texto, int x, int y, int larguraMaxima,
                                 int tamanhoFonte, int espacamento, bool desenhar){
    char linha[256] = "";
    int linhas = 0;

    while (*texto != '\0') {
        while (*texto == ' ') texto++;
        if (*texto == '\0') break;

        const char *fimPalavra = strchr(texto, ' ');
        const size_t tamanhoPalavra = fimPalavra != NULL
            ? (size_t)(fimPalavra - texto) : strlen(texto);
        if (tamanhoPalavra >= sizeof(linha)) break;

        char palavra[sizeof(linha)];
        memcpy(palavra, texto, tamanhoPalavra);
        palavra[tamanhoPalavra] = '\0';

        const size_t tamanhoLinha = strlen(linha);
        const size_t tamanhoSeparador = tamanhoLinha > 0 ? 1 : 0;
        const size_t tamanhoTentativa = tamanhoLinha + tamanhoSeparador + tamanhoPalavra;
        if (tamanhoTentativa >= sizeof(linha)) {
            if (linha[0] == '\0') break;
            if (desenhar) {
                DrawText(linha, x, y + linhas * espacamento, tamanhoFonte, (Color){214, 204, 190, 255});
            }
            linhas++;
            memcpy(linha, palavra, tamanhoPalavra + 1);
            texto += tamanhoPalavra;
            continue;
        }

        char tentativa[sizeof(linha)];
        memcpy(tentativa, linha, tamanhoLinha);
        if (tamanhoSeparador > 0) tentativa[tamanhoLinha] = ' ';
        memcpy(tentativa + tamanhoLinha + tamanhoSeparador, palavra, tamanhoPalavra + 1);

        if (linha[0] != '\0' && MeasureText(tentativa, tamanhoFonte) > larguraMaxima) {
            if (desenhar) {
                DrawText(linha, x, y + linhas * espacamento, tamanhoFonte, (Color){214, 204, 190, 255});
            }
            linhas++;
            memcpy(linha, palavra, tamanhoPalavra + 1);
        } else {
            memcpy(linha, tentativa, tamanhoTentativa + 1);
        }
        texto += tamanhoPalavra;
    }

    if (linha[0] != '\0') {
        if (desenhar) {
            DrawText(linha, x, y + linhas * espacamento, tamanhoFonte, (Color){214, 204, 190, 255});
        }
        linhas++;
    }
    return linhas;
}

void DesenharCaixaDialogo(const char *texto, const char *textoCompleto,
                          bool ultimaFala, bool falaCompleta, int larguraTela, int alturaTela){
    const int tamanhoFonte = 20;
    const int espacamento = 26;
    const float margem = 26.0f;
    const float largura = fminf((float)larguraTela - margem * 2.0f, 740.0f);
    const int larguraTexto = (int)largura - 58;
    const int linhas = DesenharTextoQuebrado(textoCompleto, 0, 0, larguraTexto, tamanhoFonte, espacamento, false);
    const float altura = 66.0f + (float)(linhas > 0 ? linhas : 1) * (float)espacamento;
    const float x = margem;
    const float y = (float)alturaTela - altura - margem;
    const Rectangle painel = {x, y, largura, altura};

    DrawRectangleRounded(painel, 0.08f, 10, (Color){91, 28, 27, 245});
    DrawRectangleRounded((Rectangle){x + 2.0f, y + 2.0f, largura - 4.0f, altura - 4.0f},
                         0.08f, 10, (Color){20, 18, 17, 248});
    DrawRectangle((int)x + 16, (int)y + 16, 3, (int)altura - 32, (Color){137, 83, 49, 255});
    DrawText("VOCÊ", (int)x + 30, (int)y + 11, 16, (Color){176, 128, 79, 255});
    DesenharTextoQuebrado(texto, (int)x + 30, (int)y + 34, larguraTexto,
                          tamanhoFonte, espacamento, true);

    if (ultimaFala && falaCompleta) {
        const char *instrucao = "ENTER - começar";
        DrawText(instrucao, (int)(x + largura - 18.0f - MeasureText(instrucao, 14)),
                 (int)(y + altura - 24.0f), 14, (Color){176, 128, 79, 255});
    } else if (!ultimaFala) {
        const char *instrucao = "ENTER - próxima fala";
        DrawText(instrucao, (int)(x + largura - 18.0f - MeasureText(instrucao, 14)),
                 (int)(y + altura - 24.0f), 14, (Color){176, 128, 79, 255});
    }
}

Rectangle CalcularDestinoTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela){
    const float escala = fmaxf((float)larguraTela / (float)imagemFundo.width, (float)alturaTela / (float)imagemFundo.height);
    const float larguraImagem = (float)imagemFundo.width * escala;
    const float alturaImagem = (float)imagemFundo.height * escala;

    return (Rectangle){
        ((float)larguraTela - larguraImagem) * 0.5f,
        ((float)alturaTela - alturaImagem) * 0.5f,
        larguraImagem, alturaImagem
    };
}

Rectangle CalcularBotaoTelaInicial(Texture2D imagemFundo, Rectangle destinoImagemFundo){
    const Rectangle areaBotaoNaImagem = {
        (float)imagemFundo.width * 0.416f,
        (float)imagemFundo.height * 0.733f,
        (float)imagemFundo.width * 0.168f,
        (float)imagemFundo.height * 0.077f
    };

    const float escalaX = destinoImagemFundo.width / (float)imagemFundo.width;
    const float escalaY = destinoImagemFundo.height / (float)imagemFundo.height;
    return (Rectangle){
        destinoImagemFundo.x + areaBotaoNaImagem.x * escalaX,
        destinoImagemFundo.y + areaBotaoNaImagem.y * escalaY,
        areaBotaoNaImagem.width * escalaX,
        areaBotaoNaImagem.height * escalaY
    };
}

void DesenharTelaInicial(Texture2D imagemFundo, int larguraTela, int alturaTela, float opacidade){
    ClearBackground((Color){1, 3, 4, 255});
    const Rectangle origemImagemFundo = {
        0.0f, 0.0f, (float)imagemFundo.width, (float)imagemFundo.height
    };
    const Rectangle destinoImagemFundo = CalcularDestinoTelaInicial(imagemFundo, larguraTela, alturaTela);
    DrawTexturePro(imagemFundo, origemImagemFundo, destinoImagemFundo,
                   (Vector2){0.0f, 0.0f}, 0.0f,
                   (Color){255, 255, 255, (unsigned char)(255.0f * opacidade)});

    const char *atalho = "R - Ranking";
    DrawText(atalho, (larguraTela - MeasureText(atalho, 20)) / 2, alturaTela - 40, 20,
             (Color){214, 204, 190, (unsigned char)(200.0f * opacidade)});
}

static void DesenharTelaSobreposta(const char *titulo, const char *const *opcoes, int quantidadeOpcoes,
                                   int larguraTela, int alturaTela){
    const int tamanhoTitulo = 48;
    const int tamanhoOpcao = 22;
    int y = alturaTela / 2 - 90;

    DrawRectangle(0, 0, larguraTela, alturaTela, (Color){0, 0, 0, 170});
    DrawText(titulo, (larguraTela - MeasureText(titulo, tamanhoTitulo)) / 2, y, tamanhoTitulo, (Color){214, 204, 190, 255});
    y += tamanhoTitulo + 28;

    for (int i = 0; i < quantidadeOpcoes; i++) {
        DrawText(opcoes[i], (larguraTela - MeasureText(opcoes[i], tamanhoOpcao)) / 2, y, tamanhoOpcao, (Color){150, 112, 72, 255});
        y += tamanhoOpcao + 12;
    }
}

void DesenharTelaPausa(int larguraTela, int alturaTela){
    const char *const opcoes[] = {"ESC - Continuar", "M - Voltar ao menu", "Q - Sair do jogo"};
    DesenharTelaSobreposta("PAUSADO", opcoes, 3, larguraTela, alturaTela);
}

void DesenharTelaFim(int larguraTela, int alturaTela){
    const char *const opcoes[] = {"ENTER - Tentar de novo", "M - Voltar ao menu"};
    DesenharTelaSobreposta("VOCÊ FOI PEGO", opcoes, 2, larguraTela, alturaTela);
}

void DesenharTelaVitoria(int larguraTela, int alturaTela, float tempo, const char *nome){
    const char *const opcoes[] = {
        TextFormat("Tempo: %s", FormatarTempo(tempo)),
        TextFormat("Seu nome: %s_", nome),
        "ENTER - Confirmar"
    };
    DesenharTelaSobreposta("VOCÊ ESCAPOU", opcoes, 3, larguraTela, alturaTela);
}
