// ============================================================================
//  widgets.cpp
// ============================================================================
#include "widgets.h"
#include "draw2d.h"

#include <cmath>

namespace panel {
namespace wg {

using namespace panel::d2d;

// ---------------------------------------------------------------- 开关
bool SwitchInput(const char* id, ImVec2 mn, ImVec2 mx, bool& hovered, bool& held)
{
    ImGui::SetCursorScreenPos(mn);
    ImGui::InvisibleButton(id, ImVec2(mx.x - mn.x, mx.y - mn.y));
    hovered = ImGui::IsItemHovered();
    held    = ImGui::IsItemActive();
    return ImGui::IsItemClicked(ImGuiMouseButton_Left);
}

void SwitchDraw(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, bool on, float& anim,
                float& vel, float press, const Theme& th, bool hovered, bool spring)
{
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    if (dt <= 0.0f) dt = 1.0f / 60.0f;
    if (dt > 0.1f)  dt = 0.1f;

    float target = on ? 1.0f : 0.0f;
    if (spring)
    {
        float stiff = 260.0f, damp = 22.0f;
        float acc = (target - anim) * stiff - vel * damp;
        vel  += acc * dt;
        anim += vel * dt;
        if (fabsf(anim - target) < 0.002f && fabsf(vel) < 0.02f) { anim = target; vel = 0.0f; }
        if (anim < -0.22f) { anim = -0.22f; vel = 0.0f; }
        if (anim > 1.22f)  { anim = 1.22f;  vel = 0.0f; }
    }
    else
    {
        anim = Approach(anim, target, 20.0f, dt);
        vel  = 0.0f;
    }

    float t  = anim < 0.0f ? 0.0f : (anim > 1.0f ? 1.0f : anim);
    float w  = mx.x - mn.x;
    float h  = mx.y - mn.y;
    float rr = h * 0.5f;

    // 轨道
    FillRoundRect(dl, mn, mx, rr, th.track);
    if (t > 0.002f)
        FillRoundRectVGradient(dl, mn, mx, rr, WithAlpha(th.accentA, t), WithAlpha(th.accentB, t));

    if (hovered)
        FillRoundRect(dl, mn, mx, rr, Rgba(255, 255, 255, 0.035f));

    // 圆钮
    float knobH  = 24.0f * k;
    float knobW  = knobH + 2.0f * k * press;
    float margin = 3.0f * k;
    float travel = w - margin * 2.0f - knobH;
    float cx     = mn.x + margin + travel * anim;
    float cy     = mn.y + (h - knobH) * 0.5f;

    ImVec2 kmn(cx, cy);
    ImVec2 kmx(cx + knobW, cy + knobH);

    ImVec2 kc((kmn.x + kmx.x) * 0.5f, (kmn.y + kmx.y) * 0.5f);
    SoftBlob(dl, ImVec2(kc.x, kc.y + 2.0f * k), knobH * 0.80f, Rgba(0, 0, 0, 0.5f), 8);

    ImU32 knobCol = LerpColor(th.knob, th.knobOn, t);
    FillRoundRect(dl, kmn, kmx, knobH * 0.5f, knobCol);
}

// ---------------------------------------------------------------- 滑杆
bool SliderInput(const char* id, ImVec2 mn, ImVec2 mx, float k, float vmin, float vmax,
                 float step, float* value, float& pop, bool& hovered, bool& active)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetCursorScreenPos(mn);
    ImGui::InvisibleButton(id, ImVec2(mx.x - mn.x, mx.y - mn.y));
    hovered = ImGui::IsItemHovered();
    active  = ImGui::IsItemActive();

    float pillW = 66.0f * k;
    float gap   = 14.0f * k;
    float x0    = mn.x;
    float x1    = mx.x - pillW - gap;

    bool changed = false;
    float range = vmax - vmin;
    if (range <= 0.0f) range = 1.0f;

    if (active && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        float mw = x1 - x0;
        if (mw > 1.0f)
        {
            float np = (io.MousePos.x - x0) / mw;
            np = np < 0.0f ? 0.0f : (np > 1.0f ? 1.0f : np);
            float nv = vmin + np * range;
            if (step > 0.0f) nv = vmin + floorf((nv - vmin) / step + 0.5f) * step;
            if (nv < vmin) nv = vmin;
            if (nv > vmax) nv = vmax;
            if (fabsf(nv - *value) > 1e-6f)
            {
                *value  = nv;
                changed = true;
                pop     = 1.0f;
            }
        }
    }
    return changed;
}

void SliderDraw(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, const Theme& th, ImFont* mono,
                float vmin, float vmax, float value, const char* label, float pop,
                bool hovered, bool active)
{
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    if (dt <= 0.0f) dt = 1.0f / 60.0f;
    if (pop > 0.0f) pop = pop - dt / 0.38f;
    if (pop < 0.0f) pop = 0.0f;

    float popScale = 1.0f;
    if (pop > 0.0f)
    {
        float q = 1.0f - pop;
        popScale = 1.0f + 0.12f * sinf(q * 3.14159265f);
    }

    float range = vmax - vmin;
    if (range <= 0.0f) range = 1.0f;
    float p = (value - vmin) / range;
    p = p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);

    float pillW = 66.0f * k;
    float gap   = 14.0f * k;
    float x0    = mn.x;
    float x1    = mx.x - pillW - gap;
    float cy    = (mn.y + mx.y) * 0.5f;

    // 轨道
    float th2 = 4.0f * k;
    ImVec2 tmn(x0, cy - th2 * 0.5f);
    ImVec2 tmx(x1, cy + th2 * 0.5f);
    FillRoundRect(dl, tmn, tmx, th2 * 0.5f, th.track);

    float fillX = x0 + (x1 - x0) * p;
    if (fillX > x0 + 0.5f)
        FillRoundRectVGradient(dl, tmn, ImVec2(fillX, tmx.y), th2 * 0.5f, th.accentB, th.accentA);

    // 圆钮
    float thumbR = 7.0f * k;
    ImVec2 thumb(fillX, cy);
    float glowR = thumbR + (active ? 5.0f : (hovered ? 4.6f : 4.0f)) * k;
    dl->AddCircleFilled(thumb, glowR, Rgba(255, 255, 255, active ? 0.26f : 0.16f), 32);
    SoftBlob(dl, ImVec2(thumb.x, thumb.y + 2.0f * k), thumbR * 1.5f, Rgba(0, 0, 0, 0.42f), 6);
    float tr = thumbR * (active ? 1.12f : 1.0f);
    dl->AddCircleFilled(thumb, tr, IM_COL32(255, 255, 255, 255), 32);

    // 数值胶囊
    float pillH = 26.0f * k * popScale;
    float pillCx = mx.x - pillW * 0.5f;
    float hw = pillW * 0.5f * popScale;
    ImVec2 pmn(pillCx - hw, cy - pillH * 0.5f);
    ImVec2 pmx(pillCx + hw, cy + pillH * 0.5f);
    FillRoundRect(dl, pmn, pmx, 8.0f * k, th.chipBg);
    StrokeRoundRect(dl, pmn, pmx, 8.0f * k, th.hair, 1.0f);
    TextCentered(dl, mono, 12.0f * k * popScale, pmn, pmx, th.accentInk, label);
}

// ---------------------------------------------------------------- 键帽
ImVec2 KbdSize(float k, ImFont* mono, const char* label)
{
    float tw = Measure(mono, 11.5f * k, label).x;
    return ImVec2(tw + 16.0f * k + 2.0f, 11.5f * k * 1.34f + 8.0f * k + 2.0f);
}

void Kbd(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, const Theme& th, ImFont* mono,
         const char* label)
{
    FillRoundRect(dl, mn, mx, 7.0f * k, th.chipBg);
    StrokeRoundRect(dl, mn, mx, 7.0f * k, th.hair, 1.0f);
    TextCentered(dl, mono, 11.5f * k, mn, mx, th.ink2, label);
}

// ---------------------------------------------------------------- 波纹
void Ripple(ImDrawList* dl, ImVec2 center, ImVec2 hostMn, ImVec2 hostMx, float t01,
            const Theme& th)
{
    float w = hostMx.x - hostMn.x;
    float h = hostMx.y - hostMn.y;
    float base = w > h ? w : h;
    float r = t01 * base * 1.7f * 0.5f;
    if (r <= 0.0f) return;
    float a = (1.0f - t01) * 0.9f;
    SoftBlob(dl, center, r, WithAlpha(th.ripple, a), 10);
}

} // namespace wg
} // namespace panel
