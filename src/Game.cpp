#include "Game.h"
#include <algorithm>
#include <cmath>

namespace platformer {
namespace {
float Approach(float value, float target, float delta) {
    return value < target ? std::min(value+delta,target) : std::max(value-delta,target);
}
}
Enemy::Enemy(float left,float right,float y,float speed)
    :x_(left),y_(y),left_(left),right_(right),speed_(speed) {}
// IA determinista basada en reglas: patrulla y cambia de direccion en limites.
void Enemy::Update(float dt) {
    x_ += direction_*speed_*dt;
    if (x_ >= right_) { x_=right_; direction_=-1; }
    if (x_ <= left_) { x_=left_; direction_=1; }
}
Level::Level() { Reset(); }
void Level::Reset() {
    // Geometria del archivo de referencia, incluida la meta y los dos vacios.
    surfaces_ = {{{0,400,600,220}},{{820,400,630,220}},{{1690,400,910,220}},
        {{310,340,55,60},true},{{550,305,100,18}},{{695,270,100,18}},
        {{1010,315,120,18}},{{1200,245,120,18}},{{1400,295,110,18}},
        {{1560,265,110,18}},{{1880,330,65,70},true},
        {{2005,285,120,18}},{{2200,315,100,18}}};
    spikes_={{440,376,80,24},{930,376,60,24},{1140,376,120,24},
             {1760,376,60,24},{2120,376,80,24}};
    enemies_={Enemy(1300,1380,376,48),Enemy(2310,2400,376,58)};
}
void Level::Update(float dt) { for (auto& e:enemies_) e.Update(dt); }
bool Level::DestroyNear(Rectangle p,int facing) {
    Rectangle reach{facing>0?p.x+p.width:p.x-48,p.y-4,48,p.height+8};
    for (auto& s:surfaces_) if (s.active && s.destructible && CheckCollisionRecs(reach,s.rect)) {
        s.active=false; return true;
    }
    return false;
}
// Igual geometria en render y colision: vertices, esquinas y cruces de aristas.
// Incluye intersecciones donde ningun vertice esta dentro del otro poligono.
bool Level::TouchesSpikes(Rectangle p) const {
    for (auto strip:spikes_) for(float x=strip.x;x<strip.x+strip.width;x+=20) {
        Vector2 triangle[]={{x+10,strip.y},{x,strip.y+strip.height},
                            {x+20,strip.y+strip.height},{x+10,strip.y}};
        Vector2 edge[]={{p.x,p.y},{p.x+p.width,p.y},{p.x+p.width,p.y+p.height},
                        {p.x,p.y+p.height},{p.x,p.y}};
        for(int v=0;v<3;++v) if(CheckCollisionPointRec(triangle[v],p)) return true;
        for(int v=0;v<4;++v) if(CheckCollisionPointTriangle(edge[v],triangle[0],triangle[1],triangle[2])) return true;
        for(int e=0;e<4;++e) for(int t=0;t<3;++t)
            if(CheckCollisionLines(edge[e],edge[e+1],triangle[t],triangle[t+1],nullptr)) return true;
    }
    return false;
}
bool Level::TouchesEnemy(Rectangle p) const {
    for(const auto& e:enemies_) if(CheckCollisionRecs(p,e.Bounds())) return true;
    return false;
}
void Player::Reset(Vector2 feet) {
    position_=feet; velocity_={}; grounded_=false; facing_=1; animationTime_=0;
}
// Integracion semiimplicita. X e Y se resuelven por separado contra todos los solidos.
// h=1/240 y |vy|<=700 limitan el avance vertical a 2.92 px por tick (<18 px).
void Player::Update(const Input& input,const std::vector<Surface>& surfaces,float dt) {
    const int direction=std::clamp(input.direction,-1,1);
    if(direction) facing_=direction;
    velocity_.x=Approach(velocity_.x,direction*240.0f,(direction?1800.0f:2200.0f)*dt);
    if(input.jump && grounded_) { velocity_.y=-450; grounded_=false; }
    const float dx=velocity_.x*dt;
    position_.x+=dx;
    for(const auto& s:surfaces) if(s.active && CheckCollisionRecs(Bounds(),s.rect)) {
        if(dx>0) position_.x=s.rect.x-16;
        else if(dx<0) position_.x=s.rect.x+s.rect.width+16;
        velocity_.x=0;
    }
    position_.x=std::clamp(position_.x,16.0f,MapWidth-16);
    velocity_.y=std::min(velocity_.y+900*dt,700.0f);
    const float dy=velocity_.y*dt;
    position_.y+=dy; grounded_=false;
    for(const auto& s:surfaces) if(s.active && CheckCollisionRecs(Bounds(),s.rect)) {
        if(dy>0) { position_.y=s.rect.y; grounded_=true; }
        else if(dy<0) position_.y=s.rect.y+s.rect.height+36;
        velocity_.y=0;
    }
    animationTime_+=dt;
}
Animation Player::Pose() const {
    if(!grounded_) return velocity_.y<0?Animation::Jump:Animation::Fall;
    return std::fabs(velocity_.x)>10?Animation::Walk:Animation::Idle;
}
int Player::Frame() const {
    const auto pose=Pose();
    if(pose==Animation::Jump || pose==Animation::Fall) return 0;
    return static_cast<int>(animationTime_*(pose==Animation::Walk?10:3))%4;
}
void Game::Reset() {
    level_.Reset(); player_.Reset(); state_=GameState::Playing; deaths_=0;
    elapsed_=0; accumulator_=0; droppedTime_=0; defeatTime_=0; noticeTime_=0;
    pendingJump_=pendingAction_=false;
}
void Game::Advance(double frameTime,Input input) {
    if(input.restart) { Reset(); return; }
    if(input.pause && (state_==GameState::Playing || state_==GameState::Paused)) {
        state_=state_==GameState::Playing?GameState::Paused:GameState::Playing;
        accumulator_=0; pendingJump_=pendingAction_=false; return;
    }
    if(state_==GameState::Paused || state_==GameState::Won) return;
    if(!std::isfinite(frameTime) || frameTime<0) return;
    // Limita recuperacion a 60 ticks para evitar espiral de trabajo tras un bloqueo.
    // Lo descartado se registra, no se presenta como tiempo real simulado.
    droppedTime_+=std::max(0.0,frameTime-0.25);
    accumulator_+=std::min(frameTime,0.25);
    pendingJump_|=input.jump; pendingAction_|=input.action;
    while(accumulator_+1e-10>=Step) {
        input.jump=pendingJump_; input.action=pendingAction_;
        pendingJump_=pendingAction_=false;
        Tick(input); accumulator_-=Step;
        if(state_==GameState::Won) { accumulator_=0; break; }
    }
}
void Game::Tick(Input input) {
    constexpr float h=static_cast<float>(Step);
    if(state_==GameState::Defeated) {
        defeatTime_-=h;
        if(defeatTime_<=0) { player_.Reset(); state_=GameState::Playing; }
        return;
    }
    elapsed_+=Step;
    noticeTime_=std::max(0.0f,noticeTime_-h);
    if(input.action && level_.DestroyNear(player_.Bounds(),player_.Facing())) noticeTime_=1.2f;
    player_.Update(input,level_.Surfaces(),h);
    level_.Update(h);
    if(player_.Position().y>700 || level_.TouchesSpikes(player_.Bounds()) || level_.TouchesEnemy(player_.Bounds())) {
        ++deaths_; state_=GameState::Defeated; defeatTime_=0.85f;
    } else if(CheckCollisionRecs(player_.Bounds(),Goal)) state_=GameState::Won;
}
}
