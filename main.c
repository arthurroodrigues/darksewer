#include "main.h"

#define LARGURA_JANELA 960
#define ALTURA_JANELA 720

int main(void){
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Dark Sewer");
    SetWindowMinSize(480, 360);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){4, 8, 8, 255});
        EndDrawing();
    }

    CloseWindow();

    return 0;
}