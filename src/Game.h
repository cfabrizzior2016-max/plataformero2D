#pragma once
#include "raylib.h"
#include <vector>

namespace platformer {
constexpr float MapWidth = 2600;
constexpr Vector2 Start{80,400};
constexpr Rectangle Goal{2480,300,60,100};
enum class GameState { Playing, Paused, Defeated, Won };
enum class Animation { Idle, Walk, Jump, Fall };
struct Input {
    int direction = 0;
    bool jump = false, action = false, pause = false, restart = false;
};
struct Surface { Rectangle rect; bool destructible = false, active = true; };

class Enemy {
public:
    Enemy(float left, float right, float y, float speed);
    void Update(float dt);
    Rectangle Bounds() const { return {x_, y_, 28, 24}; }
    int Direction() const { return direction_; }
private:
    float x_, y_, left_, right_, speed_;
    int direction_ = 1;
};

class Level {
public:
    Level();
    void Reset();
    void Update(float dt);
    bool DestroyNear(Rectangle player, int facing);
    bool TouchesSpikes(Rectangle player) const;
    bool TouchesEnemy(Rectangle player) const;
    const std::vector<Surface>& Surfaces() const { return surfaces_; }
    const std::vector<Rectangle>& Spikes() const { return spikes_; }
    const std::vector<Enemy>& Enemies() const { return enemies_; }
private:
    std::vector<Surface> surfaces_;
    std::vector<Rectangle> spikes_;
    std::vector<Enemy> enemies_;
};

class Player {
public:
    void Reset(Vector2 feet = Start);
    void Update(const Input& input, const std::vector<Surface>& surfaces, float dt);
    Rectangle Bounds() const { return {position_.x-16, position_.y-36,32,36}; }
    Vector2 Position() const { return position_; }
    Vector2 Velocity() const { return velocity_; }
    bool Grounded() const { return grounded_; }
    int Facing() const { return facing_; }
    Animation Pose() const;
    int Frame() const;
    float AnimationTime() const { return animationTime_; }
private:
    Vector2 position_ = Start, velocity_{};
    bool grounded_ = false;
    int facing_ = 1;
    float animationTime_ = 0;
};

class Game {
public:
    static constexpr double Step = 1.0/240.0;
    Game() { Reset(); }
    void Reset();
    // La entrada de flanco se retiene hasta el siguiente tick; nunca se repite.
    void Advance(double frameTime, Input input);
    const Player& GetPlayer() const { return player_; }
    const Level& GetLevel() const { return level_; }
    GameState State() const { return state_; }
    int Deaths() const { return deaths_; }
    double Elapsed() const { return elapsed_; }
    double DroppedTime() const { return droppedTime_; }
    float NoticeTime() const { return noticeTime_; }
private:
    void Tick(Input input);
    Player player_;
    Level level_;
    GameState state_ = GameState::Playing;
    int deaths_ = 0;
    double elapsed_ = 0, accumulator_ = 0, droppedTime_ = 0;
    float defeatTime_ = 0, noticeTime_ = 0;
    bool pendingJump_ = false, pendingAction_ = false;
};
}
