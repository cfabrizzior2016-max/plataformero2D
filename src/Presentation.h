#pragma once
#include "Game.h"

namespace platformer {
class CameraController {
public:
    CameraController() { Reset(); }
    void Reset();
    void Toggle() { smooth_=!smooth_; }
    void Zoom(float wheel);
    void Update(Vector2 position,float dt,bool snap=false);
    Camera2D View() const { return camera_; }
    bool Smooth() const { return smooth_; }
private:
    Camera2D camera_{};
    bool smooth_=true;
};
class Presentation {
public:
    Presentation();
    ~Presentation();
    Presentation(const Presentation&)=delete;
    Presentation& operator=(const Presentation&)=delete;
    bool Ready() const { return sprite_.id!=0; }
    void Draw(const Game& game,const CameraController& camera) const;
private:
    void DrawWorld(const Game& game) const;
    void DrawHUD(const Game& game,const CameraController& camera) const;
    Texture2D sprite_{};
};
}
