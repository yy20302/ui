// ============================================================================
//  snow.h — 面板内的飘雪
//  对应 HTML 里的 Canvas 2D 雪花层：每片有独立半径 / 速度 / 透明度 / 正弦漂移
// ============================================================================
#pragma once

#include "imgui.h"
#include <vector>

namespace panel {

class Snow
{
public:
    void  Resize(float w, float h);
    void  SetDensity(float density);                 // 密度 0..100
    void  Update(float dt, float speed, bool paused); // speed: px/s
    void  Draw(ImDrawList* dl, ImVec2 origin, float radius, ImU32 baseColor);

    int   Count() const { return (int)flakes_.size(); }

private:
    struct Flake
    {
        float x, y, r, speed, alpha, phase, sway;
    };

    Flake Make(bool spread) const;
    void  Rebuild(float density);

    std::vector<Flake> flakes_;
    float w_ = 1.0f;
    float h_ = 1.0f;
    float density_ = 45.0f;
    mutable unsigned int rng_ = 0x9E3779B9u;
};

} // namespace panel
