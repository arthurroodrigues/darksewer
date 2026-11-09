#include "main.h"

#define MAPA_ARQUIVO "assets\\mapa.png"
#define MAPA_LARGURA 768
#define MAPA_ALTURA 991
#define LARGURA_JANELA 960
#define ALTURA_JANELA 720
#define LARGURA_JANELA 960
#define ALTURA_JANELA 720

int main(void){
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Dark Sewer");
    SetWindowMinSize(480, 360);
    SetTargetFPS(60);

        if (!FileExists(MAPA_ARQUIVO)) {
        TraceLog(LOG_ERROR, "Nao encontrei mapa.png. Coloque-o ao lado de main.c.");
        CloseWindow();
        return 1;
    }

    Texture2D mapa = LoadTexture(MAPA_ARQUIVO);

    if (mapa.id == 0) {
        TraceLog(LOG_ERROR, "Falha ao carregar mapa.png.");
        CloseWindow();
        return 1;
    }

    if (mapa.width <= 0 || mapa.height <= 0) {
        TraceLog(LOG_ERROR, "mapa.png tem dimensoes invalidas (%d x %d).", mapa.width, mapa.height);
        UnloadTexture(mapa);
        CloseWindow();
        return 1;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        const Rectangle origemMapa = {0.0f, 0.0f, (float)mapa.width, (float)mapa.height};
        const Rectangle destinoMapa = {0.0f, 0.0f, (float)MAPA_LARGURA, (float)MAPA_ALTURA};
        DrawTexturePro(mapa, origemMapa, destinoMapa, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadTexture(mapa);

    CloseWindow();

    return 0;
}