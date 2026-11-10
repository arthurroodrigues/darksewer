#include "menu.h"
#include <math.h>

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
}
