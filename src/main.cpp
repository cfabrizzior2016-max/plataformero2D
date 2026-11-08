// Plataformero 2D - inicio y controles del juego.
// Logica: Game.cpp | Graficos: Presentation.cpp
#include "Game.h"
#include "Presentation.h"

int main() {
    InitWindow(800, 450, "Plataformero 2D - Escape de la zona de peligro");
    if (!IsWindowReady()) return 1;
    SetTargetFPS(60);

    {
        platformer::Game game;
        platformer::CameraController camera;
        platformer::Presentation presentation;
        if (!presentation.Ready()) {
            TraceLog(LOG_ERROR, "No se pudo cargar assets/player_sheet.png");
            CloseWindow();
            return 2;
        }

        int lastDeaths = 0;
        while (!WindowShouldClose()) {
            const float dt = GetFrameTime();
            platformer::Input input;
            input.direction = (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
                            - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
            input.jump = IsKeyPressed(KEY_SPACE);
            input.action = IsKeyPressed(KEY_E);
            input.pause = IsKeyPressed(KEY_P);
            input.restart = IsKeyPressed(KEY_R);

            game.Advance(dt, input);

            if (input.restart) {
                camera.Reset();
                lastDeaths = 0;
            }
            if (IsKeyPressed(KEY_C)) camera.Toggle();
            camera.Zoom(GetMouseWheelMove());

            // Tras una caida, la camara vuelve al inicio sin desplazamiento brusco.
            const bool respawn = game.Deaths() != lastDeaths;
            if (game.State() == platformer::GameState::Playing)
                camera.Update(game.GetPlayer().Position(), dt, respawn);
            lastDeaths = game.Deaths();

            presentation.Draw(game, camera);
        }
    }
    CloseWindow();
    return 0;
}
