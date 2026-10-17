// Juego 2D basado en el ejemplo de camara de raylib del usuario.
// Todo el juego esta en este archivo. Requiere raylib instalada (sin imagenes externas).
// Linux: g++ plataformero.cpp -std=c++11 -o plataformero -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// Windows/MinGW: g++ plataformero.cpp -std=c++11 -o plataformero.exe -lraylib -lopengl32 -lgdi32 -lwinmm
// Controles: A/D o flechas, ESPACIO para saltar, R para reiniciar,
// C para cambiar camara, rueda para zoom, ESC para salir.
#include "raylib.h"
#include "raymath.h"
#include <cmath>

#define G 900.0f
#define PLAYER_JUMP_SPD 450.0f
#define PLAYER_HOR_SPD 240.0f

struct Player { Vector2 position; float speed; bool canJump; };
struct EnvItem { Rectangle rect; int blocking; Color color; };
const float MAP_WIDTH = 2600.0f;
const Vector2 START = {80, 400};
const Rectangle GOAL = {2480, 300, 60, 100};

Rectangle PlayerRect(const Player &p)
{ return {p.position.x - 16, p.position.y - 36, 32, 36}; }

// Resolucion separada por ejes: suelo, paredes y techo son solidos.
void UpdatePlayer(Player *p, const EnvItem *items, int count, float dt, bool jump)
{
    if (jump && p->canJump) { p->speed = -PLAYER_JUMP_SPD; p->canJump = false; }
    const int direction = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
                        - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
    const float dx = direction * PLAYER_HOR_SPD * dt;
    p->position.x += dx;
    for (int i = 0; i < count; ++i) {
        if (!items[i].blocking) continue;
        const Rectangle r = items[i].rect;
        if (CheckCollisionRecs(PlayerRect(*p), r)) {
            if (dx > 0) p->position.x = r.x - 16;
            if (dx < 0) p->position.x = r.x + r.width + 16;
        }
    }
    p->position.x = Clamp(p->position.x, 16, MAP_WIDTH - 16);
    p->speed += G * dt;
    const float dy = p->speed * dt;
    p->position.y += dy;
    p->canJump = false;
    for (int i = 0; i < count; ++i) {
        if (!items[i].blocking) continue;
        const Rectangle r = items[i].rect;
        if (CheckCollisionRecs(PlayerRect(*p), r)) {
            if (dy > 0) { p->position.y = r.y; p->canJump = true; }
            else if (dy < 0) p->position.y = r.y + r.height + 36;
            p->speed = 0;
        }
    }
}

void UpdateCamera(Camera2D *camera, const Player &p, float dt, int mode)
{
    const Vector2 target = {p.position.x, p.position.y - 70};
    camera->offset = {400, 225};
    camera->target = mode == 0 ? target : Vector2Lerp(camera->target, target, 1 - std::exp(-8 * dt));
    const float halfWidth = 400 / camera->zoom;
    const float halfHeight = 225 / camera->zoom;
    camera->target.x = Clamp(camera->target.x, halfWidth, MAP_WIDTH - halfWidth);
    camera->target.y = Clamp(camera->target.y, halfHeight - 200, 650 - halfHeight);
}

// Cada pincho usa colision triangular, igual que su dibujo.
bool TouchesSpikes(Rectangle player, const Rectangle *spikes, int count)
{
    for (int i = 0; i < count; ++i) {
        for (float x = spikes[i].x; x < spikes[i].x + spikes[i].width; x += 20) {
            Vector2 tip = {x + 10, spikes[i].y};
            Vector2 left = {x, spikes[i].y + spikes[i].height};
            Vector2 right = {x + 20, spikes[i].y + spikes[i].height};
            if (CheckCollisionPointRec(tip, player) || CheckCollisionPointRec(left, player)
                || CheckCollisionPointRec(right, player)) return true;
            Vector2 corners[] = {{player.x, player.y}, {player.x + player.width, player.y},
                {player.x, player.y + player.height}, {player.x + player.width, player.y + player.height}};
            for (Vector2 corner : corners)
                if (CheckCollisionPointTriangle(corner, tip, left, right)) return true;
            Vector2 a = {player.x, player.y}, b = {player.x + player.width, player.y};
            Vector2 c = {player.x + player.width, player.y + player.height}, d = {player.x, player.y + player.height};
            Vector2 edges[] = {a, b, c, d, a};
            Vector2 triangle[] = {tip, left, right, tip};
            for (int e = 0; e < 4; ++e)
                for (int t = 0; t < 3; ++t)
                    if (CheckCollisionLines(edges[e], edges[e + 1], triangle[t], triangle[t + 1], nullptr)) return true;
        }
    }
    return false;
}

int main()
{
    InitWindow(800, 450, "Plataformero 2D - Llega a la meta");
    SetTargetFPS(60);
    Player player = {START, 0, true};
    EnvItem envItems[] = {
        {{0, 400, 600, 220}, 1, DARKGRAY},
        {{820, 400, 630, 220}, 1, DARKGRAY},
        {{1690, 400, 910, 220}, 1, DARKGRAY},
        {{310, 340, 55, 60}, 1, BROWN}, // Obstaculo que se debe saltar.
        {{550, 305, 100, 18}, 1, GRAY},
        {{695, 270, 100, 18}, 1, GRAY}, // Cruce del primer vacio.
        {{1010, 315, 120, 18}, 1, GRAY},
        {{1200, 245, 120, 18}, 1, GRAY},
        {{1400, 295, 110, 18}, 1, GRAY},
        {{1560, 265, 110, 18}, 1, GRAY}, // Cruce del segundo vacio.
        {{1880, 330, 65, 70}, 1, BROWN},
        {{2005, 285, 120, 18}, 1, GRAY},
        {{2200, 315, 100, 18}, 1, GRAY}
    };
    Rectangle spikes[] = {{440, 376, 80, 24}, {930, 376, 60, 24},
                          {1140, 376, 120, 24}, {1760, 376, 60, 24}, {2120, 376, 80, 24}};
    const int count = sizeof(envItems) / sizeof(envItems[0]);
    const int spikeCount = sizeof(spikes) / sizeof(spikes[0]);
    Camera2D camera = {};
    camera.zoom = 1;
    camera.target = START;
    int cameraMode = 1, deaths = 0;
    bool won = false;
    float elapsed = 0;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_R)) {
            player = {START, 0, true}; won = false; deaths = 0; elapsed = 0;
            camera.zoom = 1; camera.target = START;
        }
        if (IsKeyPressed(KEY_C)) cameraMode = 1 - cameraMode;
        camera.zoom = Clamp(camera.zoom + GetMouseWheelMove() * 0.05f, 0.8f, 1.6f);
        if (!won) {
            elapsed += dt;
            // Subpasos reducen el riesgo de atravesar plataformas delgadas.
            int steps = static_cast<int>(ceilf(dt / (1.0f / 240)));
            if (steps < 1) steps = 1;
            const bool jump = IsKeyPressed(KEY_SPACE);
            for (int s = 0; s < steps; ++s) {
                UpdatePlayer(&player, envItems, count, dt / steps, jump && s == 0);
                if (player.position.y > 700 || TouchesSpikes(PlayerRect(player), spikes, spikeCount)) {
                    ++deaths; player = {START, 0, true}; camera.target = START; break;
                }
                if (CheckCollisionRecs(PlayerRect(player), GOAL)) { won = true; break; }
            }
        }
        UpdateCamera(&camera, player, dt, cameraMode);
        BeginDrawing();
        ClearBackground(Color{185, 220, 245, 255});
        BeginMode2D(camera);
        for (int x = 150; x < 2600; x += 400) {
            DrawCircle(x, 100, 32, RAYWHITE);
            DrawCircle(x + 35, 100, 25, RAYWHITE);
        }
        for (int i = 0; i < count; ++i) {
            DrawRectangleRec(envItems[i].rect, envItems[i].color);
            Rectangle top = envItems[i].rect; top.height = 5;
            DrawRectangleRec(top, LIME);
        }
        DrawText("VACIO", 665, 520, 20, DARKBLUE);
        DrawText("VACIO", 1510, 520, 20, DARKBLUE);
        for (int i = 0; i < spikeCount; ++i)
            for (float x = spikes[i].x; x < spikes[i].x + spikes[i].width; x += 20)
                DrawTriangle({x + 10, spikes[i].y}, {x, spikes[i].y + spikes[i].height},
                             {x + 20, spikes[i].y + spikes[i].height}, RED);
        DrawRectangle(30, 320, 110, 32, DARKBLUE);
        DrawText("INICIO", 43, 325, 22, WHITE);
        DrawLine(2480, 400, 2480, 250, DARKGREEN);
        DrawTriangle({2480, 250}, {2480, 300}, {2550, 275}, GREEN);
        DrawRectangle(2420, 210, 140, 35, DARKGREEN);
        DrawText("META / FIN", 2430, 217, 22, WHITE);
        Rectangle body = PlayerRect(player);
        DrawRectangleRec(body, MAROON);
        DrawRectangle(static_cast<int>(body.x + 5), static_cast<int>(body.y + 7), 7, 7, WHITE);
        DrawRectangle(static_cast<int>(body.x + 20), static_cast<int>(body.y + 7), 7, 7, WHITE);
        EndMode2D();
        DrawRectangle(0, 0, 800, 76, Fade(DARKBLUE, 0.9f));
        DrawText("A/D o flechas: mover | ESPACIO: saltar | R: reiniciar", 16, 10, 18, WHITE);
        DrawText(TextFormat("Caidas: %d | Tiempo: %.1f s | C: camara %s | Rueda: zoom", deaths,
                 elapsed, cameraMode ? "suave" : "centrada"), 16, 35, 17, WHITE);
        DrawText("Evita los pinchos y el vacio. Llega a la bandera verde.", 16, 57, 14, SKYBLUE);
        if (won) {
            DrawRectangle(100, 145, 600, 165, Fade(DARKGREEN, 0.95f));
            DrawText("LLEGASTE A LA META!", 175, 175, 32, WHITE);
            DrawText(TextFormat("Tiempo: %.1f s  |  Caidas: %d", elapsed, deaths), 215, 225, 22, WHITE);
            DrawText("Presiona R para jugar de nuevo", 200, 265, 22, YELLOW);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
