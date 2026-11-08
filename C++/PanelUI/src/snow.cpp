// ============================================================================
//  snow.cpp
// ============================================================================
#include "snow.h"
#include "theme.h"

#include <cmath>

namespace panel {

namespace {
inline float Rand01(unsigned int& s)
{
    s = s * 1664525u + 1013904223u;
    return (float)((s >> 8) & 0xFFFF) / 65535.0f;
}
} // namespace

Snow::Flake Snow::Make(bool spread) const
{
    Flake f;
    unsigned int& s = rng_;
    f.x     = Rand01(s) * w_;
    f.y     = spread ? Rand01(s) * h_ : -12.0f;
    f.r     = 0.6f + Rand01(s) * 1.8f;
    f.speed = 7.0f + Rand01(s) * 16.0f;
    f.alpha = 0.16f + Rand01(s) * 0.42f;
    f.phase = Rand01(s) * 6.2831853f;
    f.sway  = 0.25f + Rand01(s) * 0.85f;
    return f;
}

void Snow::Resize(float w, float h)
{
    if (fabsf(w - w_) < 0.5f && fabsf(h - h_) < 0.5f) return;
    w_ = w > 1.0f ? w : 1.0f;
    h_ = h > 1.0f ? h : 1.0f;
    Rebuild(density_);
}

void Snow::SetDensity(float density)
{
    if (fabsf(density - density_) < 0.5f) return;
    Rebuild(density);
}

void Snow::Rebuild(float density)
{
    density_ = density;
    int n = (int)((w_ * h_) / 5200.0f * (density / 100.0f) + 0.5f);
    if (n < 0) n = 0;
    if (n > 260) n = 260;

    flakes_.clear();
    flakes_.reserve(n);
    for (int i = 0; i < n; ++i) flakes_.push_back(Make(true));
}

void Snow::Update(float dt, float speed, bool paused)
{
    if (paused || dt <= 0.0f) return;

    float factor = speed / 18.0f;
    for (size_t i = 0; i < flakes_.size(); ++i)
    {
        Flake& f = flakes_[i];
        f.phase += dt * 0.7f;
        f.y += f.speed * factor * dt;
        f.x += sinf(f.phase) * f.sway * 16.0f * dt;

        if (f.y > h_ + 12.0f || f.x < -16.0f || f.x > w_ + 16.0f)
            flakes_[i] = Make(false);
    }
}

void Snow::Draw(ImDrawList* dl, ImVec2 origin, float radius, ImU32 baseColor)
{
    if (flakes_.empty()) return;

    float baseA = AlphaOf(baseColor);
    for (size_t i = 0; i < flakes_.size(); ++i)
    {
        const Flake& f = flakes_[i];
        ImVec2 p(origin.x + f.x, origin.y + f.y);

        // 圆角区域外不画（模拟面板的 overflow:hidden）
        if (radius > 0.0f)
        {
            float cx = p.x < origin.x + radius ? origin.x + radius : (p.x > origin.x + w_ - radius ? origin.x + w_ - radius : p.x);
            float cy = p.y < origin.y + radius ? origin.y + radius : (p.y > origin.y + h_ - radius ? origin.y + h_ - radius : p.y);
            float dx = p.x - cx, dy = p.y - cy;
            if (dx * dx + dy * dy > radius * radius) continue;
        }

        dl->AddCircleFilled(p, f.r, WithAlpha(baseColor, f.alpha * baseA), 0);
    }
}

} // namespace panel
