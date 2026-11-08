// ============================================================================
//  draw2d.cpp
// ============================================================================
#include "draw2d.h"
#include "theme.h"

#include <cmath>
#include <cfloat>

namespace panel {
namespace d2d {

// ------------------------------------------------------------------ 基础
float Saturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
float Lerp(float a, float b, float t) { return a + (b - a) * t; }

float Approach(float cur, float target, float speed, float dt)
{
    if (speed <= 0.0f) return target;
    float k = 1.0f - expf(-speed * dt);
    return cur + (target - cur) * Saturate(k);
}

// ------------------------------------------------------------------ 缓动
static float BezierAxis(float t, float p1, float p2)
{
    float u = 1.0f - t;
    return 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t;
}

float CubicBezier(float x1, float y1, float x2, float y2, float x)
{
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;

    // 牛顿迭代求参数 t，使 BezierAxis(t, x1, x2) == x
    float t = x;
    for (int i = 0; i < 8; ++i)
    {
        float cx = BezierAxis(t, x1, x2) - x;
        if (fabsf(cx) < 1e-5f) break;

        float u = 1.0f - t;
        float dx = 3.0f * u * u * x1 + 6.0f * u * t * (x2 - x1) + 3.0f * t * t * (1.0f - x2);
        if (fabsf(dx) < 1e-6f) break;
        t -= cx / dx;
        t = Saturate(t);
    }
    return BezierAxis(t, y1, y2);
}

float EaseRise(float t)   { return CubicBezier(0.16f, 1.0f, 0.30f, 1.0f, Saturate(t)); }
float EaseSpring(float t) { return CubicBezier(0.34f, 1.40f, 0.64f, 1.0f, Saturate(t)); }
float EaseOutQuad(float t) { return CubicBezier(0.40f, 0.0f, 0.20f, 1.0f, Saturate(t)); }

// -------------------------------------------------------------- 圆角矩形
// 计算某一行在圆角矩形内的左右内缩量
static float CornerInset(float y, float top, float bottom, float r)
{
    if (r <= 0.0f) return 0.0f;
    float inset = 0.0f;
    if (y < top + r)
    {
        float dy = (top + r) - y;
        float d  = r * r - dy * dy;
        inset = r - (d > 0.0f ? sqrtf(d) : 0.0f);
    }
    else if (y > bottom - r)
    {
        float dy = y - (bottom - r);
        float d  = r * r - dy * dy;
        inset = r - (d > 0.0f ? sqrtf(d) : 0.0f);
    }
    return inset;
}

// 渐变区间 [gy0, gy1] 可独立于矩形范围，用于局部高光
static void FillRoundRectGradientEx(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float r,
                                    float gy0, float gy1, ImU32 c0, ImU32 c1)
{
    if (mx.x <= mn.x || mx.y <= mn.y) return;

    float h = mx.y - mn.y;
    if (r * 2.0f > h) r = h * 0.5f;
    if (r * 2.0f > (mx.x - mn.x)) r = (mx.x - mn.x) * 0.5f;

    float span = (gy1 - gy0);
    if (span <= 0.0f) span = 1.0f;

    // 逐行绘制：1px 步长贴合圆角，中段用大步长减少顶点
    float y = mn.y;
    while (y < mx.y - 0.001f)
    {
        float step = 1.0f;
        bool inCorner = (y < mn.y + r) || (y > mx.y - r);
        if (!inCorner) step = 2.0f;
        if (y + step > mx.y) step = mx.y - y;

        float yc   = y + step * 0.5f;
        float ins  = CornerInset(yc, mn.y, mx.y, r);
        float t    = Saturate((yc - gy0) / span);
        ImU32 col  = panel::LerpColor(c0, c1, t);

        dl->AddRectFilled(ImVec2(mn.x + ins, y), ImVec2(mx.x - ins, y + step + 0.5f), col);
        y += step;
    }
}

void FillRoundRectVGradient(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding,
                            ImU32 top, ImU32 bot)
{
    FillRoundRectGradientEx(dl, mn, mx, rounding, mn.y, mx.y, top, bot);
}

void FillRoundRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col)
{
    dl->AddRectFilled(mn, mx, col, rounding);
}

void StrokeRoundRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                     float thickness)
{
    dl->AddRect(mn, mx, col, rounding, 0, thickness);
}

void SoftShadow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                float spread, int layers)
{
    if (layers < 1) layers = 1;
    float baseA = panel::AlphaOf(col);
    for (int i = layers; i >= 1; --i)
    {
        float k = (float)i / (float)layers;      // 1 = 最外圈
        float e = spread * k;
        float a = baseA * (1.0f - k) * (1.0f - k) * 0.55f;
        if (a <= 0.001f) continue;
        FillRoundRect(dl, ImVec2(mn.x - e, mn.y - e), ImVec2(mx.x + e, mx.y + e),
                      rounding + e, panel::WithAlpha(col, a));
    }
}

void TopInnerHighlight(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float rounding, ImU32 col,
                       float height)
{
    // 只内缩上方圆角，下方是开放的（不能把条的底边也当成圆角处理）
    ImU32 clear = panel::WithAlpha(col, 0.0f);
    float r = rounding;
    if (r > (mx.x - mn.x) * 0.5f) r = (mx.x - mn.x) * 0.5f;

    float y0 = mn.y;
    float y1 = mn.y + height;
    float y  = y0;
    while (y < y1 - 0.001f)
    {
        float step = (y < mn.y + r) ? 1.0f : 2.0f;
        if (y + step > y1) step = y1 - y;
        float yc  = y + step * 0.5f;
        float ins = 0.0f;
        if (yc < mn.y + r)
        {
            float dy = (mn.y + r) - yc;
            float d  = r * r - dy * dy;
            ins = r - (d > 0.0f ? sqrtf(d) : 0.0f);
        }
        ImU32 c = panel::LerpColor(col, clear, Saturate((yc - y0) / (y1 - y0)));
        dl->AddRectFilled(ImVec2(mn.x + ins, y), ImVec2(mx.x - ins, y + step + 0.5f), c);
        y += step;
    }
}

// ---------------------------------------------------------------- 圆弧/线
void StrokeArc(ImDrawList* dl, ImVec2 center, float radius, float a0, float a1,
               ImU32 col, float thickness, int segments)
{
    if (segments <= 0)
        segments = (int)(fabsf(a1 - a0) / 3.14159265f * 18.0f) + 2;
    dl->PathArcTo(center, radius, a0, a1, segments);
    dl->PathStroke(col, 0, thickness);
}

void StrokeCircle(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, float thickness)
{
    dl->AddCircle(center, radius, col, 0, thickness);
}

void StrokePolyline(ImDrawList* dl, const ImVec2* pts, int count, ImU32 col,
                    float thickness, bool closed)
{
    if (count < 2) return;
    for (int i = 0; i < count; ++i) dl->PathLineTo(pts[i]);
    dl->PathStroke(col, closed ? ImDrawFlags_Closed : 0, thickness);
}

void FillHalfCircle(ImDrawList* dl, ImVec2 center, float radius, float a0, float a1, ImU32 col)
{
    dl->PathArcTo(center, radius, a0, a1, 24);
    dl->PathFillConvex(col);
}

void SoftBlob(ImDrawList* dl, ImVec2 center, float radius, ImU32 col, int segments)
{
    if (segments < 4) segments = 4;
    float baseA = panel::AlphaOf(col);
    for (int i = segments; i >= 1; --i)
    {
        float k = (float)i / (float)segments;
        float a = baseA * (1.0f - k) * (1.0f - k) * 0.5f;
        if (a <= 0.002f) continue;
        dl->AddCircleFilled(center, radius * k, panel::WithAlpha(col, a), 0);
    }
}

// -------------------------------------------------------------- 文字工具
ImVec2 Measure(ImFont* font, float size, const char* text)
{
    if (!font || !text || !*text) return ImVec2(0.0f, size);
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
}

float LineHeight(ImFont* font, float size)
{
    (void)font;
    return size * 1.32f;
}

// 极简 UTF-8 解码，返回码点并前进指针
static unsigned int NextCodepoint(const char** p)
{
    const unsigned char* s = (const unsigned char*)*p;
    unsigned int c = s[0];
    int extra = 0;
    if (c < 0x80)        { extra = 0; c = c; }
    else if ((c & 0xE0) == 0xC0) { extra = 1; c &= 0x1F; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; c &= 0x0F; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; c &= 0x07; }
    else { (*p) += 1; return 0xFFFD; }

    for (int i = 1; i <= extra; ++i)
    {
        if (!s[i]) { (*p) += 1 + i - 1; return c; }
        c = (c << 6) | (s[i] & 0x3F);
    }
    *p += 1 + extra;
    return c;
}

static void EncodeUtf8(unsigned int c, char out[5])
{
    if (c < 0x80) { out[0] = (char)c; out[1] = 0; }
    else if (c < 0x800)
    {
        out[0] = (char)(0xC0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 0x3F));
        out[2] = 0;
    }
    else
    {
        out[0] = (char)(0xE0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[2] = (char)(0x80 | (c & 0x3F));
        out[3] = 0;
    }
}

float TextSpaced(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col,
                 const char* text, float spacing)
{
    if (!text || !*text) return 0.0f;
    float x = pos.x;
    const char* p = text;
    while (*p)
    {
        char buf[5];
        unsigned int cp = NextCodepoint(&p);
        if (cp == 0) break;
        EncodeUtf8(cp, buf);
        dl->AddText(font, size, ImVec2(x, pos.y), col, buf);
        x += Measure(font, size, buf).x + spacing;
    }
    return x - pos.x - (spacing > 0.0f ? spacing : 0.0f);
}

void Text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text)
{
    if (!text || !*text) return;
    dl->AddText(font, size, pos, col, text);
}

void TextCentered(ImDrawList* dl, ImFont* font, float size, ImVec2 mn, ImVec2 mx,
                  ImU32 col, const char* text)
{
    if (!text || !*text) return;
    ImVec2 s = Measure(font, size, text);
    float x = mn.x + ((mx.x - mn.x) - s.x) * 0.5f;
    float y = mn.y + ((mx.y - mn.y) - s.y) * 0.5f;
    dl->AddText(font, size, ImVec2(x, y), col, text);
}

} // namespace d2d
} // namespace panel
