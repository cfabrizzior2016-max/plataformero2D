#include "Presentation.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace platformer {
namespace {
constexpr Color Navy{20,33,54,255},Teal{49,180,160,255},Sand{243,201,112,255};
void Label(const char* text,int x,int y,int size,Color color) { DrawText(text,x,y,size,color); }
void Panel(const char* title,const char* line,const char* hint,Color accent) {
    DrawRectangle(0,0,800,450,Fade(Navy,.66f));
    DrawRectangleRounded({150,136,500,186},.08f,8,Navy);
    DrawRectangle(150,136,6,186,accent);
    Label(title,182,160,30,accent); Label(line,182,211,19,RAYWHITE);
    Label(hint,182,258,17,Sand);
}
}
void CameraController::Reset() {
    camera_={}; camera_.zoom=1; camera_.offset={400,225}; smooth_=true;
    Update(Start,0,true);
}
void CameraController::Zoom(float wheel) {
    camera_.zoom=std::clamp(camera_.zoom+wheel*.05f,.8f,1.6f);
    // El zoom debe respetar limites incluso en pausa, cuando no hay Update.
    const float hw=400/camera_.zoom,hh=225/camera_.zoom;
    camera_.target.x=std::clamp(camera_.target.x,hw,MapWidth-hw);
    camera_.target.y=std::clamp(camera_.target.y,-200+hh,650-hh);
}
void CameraController::Update(Vector2 position,float dt,bool snap) {
    Vector2 target{position.x,position.y-70};
    const float blend=(!smooth_||snap)?1:1-std::exp(-8*dt);
    camera_.target.x+=(target.x-camera_.target.x)*blend;
    camera_.target.y+=(target.y-camera_.target.y)*blend;
    const float hw=400/camera_.zoom,hh=225/camera_.zoom;
    camera_.target.x=std::clamp(camera_.target.x,hw,MapWidth-hw);
    camera_.target.y=std::clamp(camera_.target.y,-200+hh,650-hh);
}
Presentation::Presentation() {
    const std::string path=std::string(GetApplicationDirectory())+"assets/player_sheet.png";
    sprite_=LoadTexture(path.c_str());
    if(sprite_.id) SetTextureFilter(sprite_,TEXTURE_FILTER_POINT);
}
Presentation::~Presentation() { if(sprite_.id) UnloadTexture(sprite_); }
void Presentation::DrawWorld(const Game& game) const {
    const auto& level=game.GetLevel();
    for(int x=0;x<2600;x+=320) {
        DrawTriangle({static_cast<float>(x-120),400},{static_cast<float>(x+380),400},
                     {static_cast<float>(x+130),90},Color{144,191,192,255});
        DrawCircle(x+150,75,27,Color{231,242,226,255});
        DrawCircle(x+180,75,21,Color{231,242,226,255});
    }
    for(const auto& s:level.Surfaces()) if(s.active) {
        DrawRectangleRec(s.rect,s.destructible?Color{153,86,64,255}:Color{52,79,86,255});
        Rectangle top=s.rect; top.height=5;
        DrawRectangleRec(top,s.destructible?Sand:Teal);
        if(s.destructible) {
            DrawRectangleLinesEx(s.rect,2,Sand);
            DrawLineEx({s.rect.x+5,s.rect.y+8},{s.rect.x+s.rect.width-5,s.rect.y+s.rect.height-6},2,Sand);
            Label("E",static_cast<int>(s.rect.x+20),static_cast<int>(s.rect.y+20),18,RAYWHITE);
        } else if(s.rect.height>30) {
            for(float x=s.rect.x+12;x<s.rect.x+s.rect.width-8;x+=32)
                DrawRectangle(static_cast<int>(x),static_cast<int>(s.rect.y+18),12,4,Color{68,97,101,255});
        }
    }
    for(auto r:level.Spikes()) for(float x=r.x;x<r.x+r.width;x+=20) {
        DrawTriangle({x+10,r.y},{x,r.y+r.height},{x+20,r.y+r.height},Color{222,75,86,255});
        DrawLineEx({x+10,r.y+4},{x+3,r.y+r.height-2},2,Color{255,180,161,255});
    }
    for(const auto& enemy:level.Enemies()) {
        auto b=enemy.Bounds(); DrawRectangleRounded(b,.25f,4,Color{120,58,89,255});
        DrawRectangle(static_cast<int>(b.x+4),static_cast<int>(b.y+6),20,6,Sand);
        DrawCircle(static_cast<int>(b.x+7),static_cast<int>(b.y+24),4,Navy);
        DrawCircle(static_cast<int>(b.x+21),static_cast<int>(b.y+24),4,Navy);
    }
    Label("INICIO",40,330,20,Navy); Label("VACIO",665,520,20,Navy); Label("VACIO",1510,520,20,Navy);
    DrawLineEx({2480,400},{2480,250},4,Teal);
    DrawTriangle({2480,250},{2480,300},{2550,275},Teal);
    Label("META / FIN",2420,215,20,Navy);
    const auto& p=game.GetPlayer();
    Rectangle source{static_cast<float>(p.Frame()*32),static_cast<float>(static_cast<int>(p.Pose())*40),32,40};
    if(p.Facing()<0) source.width=-32;
    const auto pos=p.Position();
    DrawTexturePro(sprite_,source,{pos.x-16,pos.y-40,32,40},{0,0},0,WHITE);
}
void Presentation::DrawHUD(const Game& game,const CameraController& camera) const {
    DrawRectangle(0,0,800,111,Fade(Navy,.97f));
    Label("ESCAPE DE LA ZONA DE PELIGRO",16,9,20,Sand);
    Label("A/D o flechas  Mover    ESPACIO  Saltar    E  Romper cajas",16,35,16,RAYWHITE);
    Label("P  Pausa    R  Reiniciar    C  Camara    Rueda  Zoom    ESC  Salir",16,59,15,RAYWHITE);
    Label(TextFormat("Caidas %d    Tiempo %.1f s    Camara %s    Zoom %.2f",game.Deaths(),
          game.Elapsed(),camera.Smooth()?"suave":"centrada",camera.View().zoom),16,86,16,RAYWHITE);
    if(game.NoticeTime()>0) Label("Caja destruida",320,119,18,Navy);
    switch(game.State()) {
    case GameState::Paused: Panel("PAUSA","El mundo y el tiempo estan detenidos.","P  Continuar    R  Reiniciar    ESC  Salir",Sand); break;
    case GameState::Defeated: Panel("DERROTA","Peligro detectado. Volviendo al inicio...","R  Reiniciar partida    ESC  Salir",Color{255,129,128,255}); break;
    case GameState::Won: Panel("LLEGASTE A LA META",TextFormat("Tiempo %.1f s    Caidas %d",game.Elapsed(),game.Deaths()),"R  Jugar de nuevo    ESC  Salir",Teal); break;
    default: break;
    }
}
void Presentation::Draw(const Game& game,const CameraController& camera) const {
    BeginDrawing(); ClearBackground(Color{194,223,218,255});
    BeginMode2D(camera.View()); DrawWorld(game); EndMode2D(); DrawHUD(game,camera);
    EndDrawing();
}
}
