// ============================================================================
//  widgets.h — 自绘控件（开关 / 滑杆 / 键帽）
//  全部手绘以还原 CSS 里的玻璃质感。
//  输入与绘制分开：先 Input 拿到交互结果，更新数值后再 Draw，
//  这样滑杆拖动的当帧就能显示新数值。
// ============================================================================
#pragma once

#include "imgui.h"
#include "theme.h"

namespace panel {
namespace wg {

// ---- 开关 ----
bool SwitchInput(const char* id, ImVec2 mn, ImVec2 mx, bool& hovered, bool& held);
void SwitchDraw(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, bool on, float& anim,
                float& vel, float press, const Theme& th, bool hovered, bool spring);

// ---- 滑杆 + 数值胶囊 ----
bool SliderInput(const char* id, ImVec2 mn, ImVec2 mx, float k, float vmin, float vmax,
                 float step, float* value, float& pop, bool& hovered, bool& active);
void SliderDraw(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, const Theme& th, ImFont* mono,
                float vmin, float vmax, float value, const char* label, float pop,
                bool hovered, bool active);

// ---- 键帽 ----
ImVec2 KbdSize(float k, ImFont* mono, const char* label);
void   Kbd(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float k, const Theme& th, ImFont* mono,
           const char* label);

// ---- 点击波纹 ----
void Ripple(ImDrawList* dl, ImVec2 center, ImVec2 hostMn, ImVec2 hostMx, float t01,
            const Theme& th);

} // namespace wg
} // namespace panel
