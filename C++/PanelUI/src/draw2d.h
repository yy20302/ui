// ============================================================================
//  draw2d.h — 缓动函数与手绘图元
//  ImGui 的 ImDrawList 只有基础图元，这里补齐 CSS 里用到的能力：
//  圆角垂直渐变填充、柔和投影、圆弧描边、字距、软光斑
// ============================================================================
#pragma once

#include "imgui.h"

namespace panel {
namespace d2d {

// ------------------------------------------------------------------ 缓动
// CSS cubic-bezier(x1,y1,x2,y2) 求值（牛顿迭代解 x 再取 y）
float CubicBezier(float x1, float y1, float x2, float y2, float x);

// 常用曲线，名字对应 CSS 里的定义
float EaseRise(float t);      // cubic-bezier(.16,1,.3,1)   面板/行入场
float EaseSpring(float t);    // cubic-bezier(.34,1.4,.64,1) 开关回弹
float EaseOutQuad(float t);   // cubic-bezier(.4,0,.2,1)     背景过渡

float Saturate(float v);
float Lerp(float a, float b, float t);

// 帧率无关的指数趋近（用于 hover / 动画插值）
float Approach(float cur, float target, float speed, float dt);

// -------------------------------------------------------------- 圆角矩形
// 垂直渐变填充圆角矩形：逐行内缩贴合圆角，保证与 CSS 的圆角一致
void FillRoundRectVGradient(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding,
                            ImU32 top, ImU32 bot);

// 单色圆角填充（带 AA）
void FillRoundRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col);

// 圆角描边（1px 级细线，AA）
void StrokeRoundRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                     float thickness = 1.0f);

// 柔和外投影：多层递减 alpha 的圆角矩形堆叠
void SoftShadow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                float spread, int layers = 10);

// 内高光：顶部一条渐变亮线
void TopInnerHighlight(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                       float height);

// ---------------------------------------------------------------- 圆弧/线
void StrokeArc(ImDrawList* dl, ImVec2 center, float radius, float a0, float a1,
               ImU32 col, float thickness, int segments = 0);

void StrokeCircle(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, float thickness);

// 折线（closed 时首尾相连），内部使用 PathStroke 以获得圆角接头
void StrokePolyline(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col,
                    float thickness, bool closed);

// 半圆填充（图标用）
void FillHalfCircle(ImDrawList* dl, ImVec2 center, float radius, float a0, float a1, ImU32 col);

// -------------------------------------------------------------- 文字工具
ImVec2 Measure(ImFont* font, float size, const char* text);
float  LineHeight(ImFont* font, float size);

// 带字距的文本（CSS letter-spacing），返回绘制后的宽度
float TextSpaced(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col,
                 const char* text, float spacing);

// 单行文本，pos.y 为行盒顶部
void Text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text);

// 在矩形内水平居中绘制
void TextCentered(ImDrawList* dl, ImFont* font, float size, ImVec2 mn, ImVec2 mx,
                  ImU32 col, const char* text);

// 软光斑：中心亮、边缘透明（radial-gradient 近似）
void SoftBlob(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, int segments = 24);

} // namespace d2d
} // namespace panel
