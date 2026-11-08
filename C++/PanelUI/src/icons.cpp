// ============================================================================
//  icons.cpp
// ============================================================================
#include "icons.h"
#include "draw2d.h"
#include "theme.h"

#include <cmath>

namespace panel {

namespace {

const float kPi = 3.14159265358979323846f;

struct IconSpace
{
    ImVec2 c;
    float  s;        // 24 -> size 的缩放
    float  thick;

    ImVec2 P(float x, float y) const
    {
        return ImVec2(c.x + (x - 12.0f) * s, c.y + (y - 12.0f) * s);
    }
};

// 在 24x24 空间里描一条折线
void Poly(ImDrawList* dl, const IconSpace& sp, const float* xy, int n, ImU32 col, bool closed)
{
    ImVec2 pts[64];
    if (n > 64) n = 64;
    for (int i = 0; i < n; ++i) pts[i] = sp.P(xy[i * 2], xy[i * 2 + 1]);
    d2d::StrokePolyline(dl, pts, n, col, sp.thick, closed);
}

// 24x24 空间里的圆弧，角度按屏幕坐标（y 向下）
void Arc(ImDrawList* dl, const IconSpace& sp, float cx, float cy, float r,
         float a0, float a1, ImU32 col)
{
    ImVec2 pts[40];
    int n = 0;
    for (int i = 0; i < 24; ++i)
    {
        float t = (float)i / 23.0f;
        float a = a0 + (a1 - a0) * t;
        pts[n++] = sp.P(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    d2d::StrokePolyline(dl, pts, n, col, sp.thick, false);
}

} // namespace

void DrawIcon(ImDrawList* dl, IconId id, ImVec2 center, float size, ImU32 col)
{
    IconSpace sp;
    sp.c     = center;
    sp.s     = size / 24.0f;
    sp.thick = 1.6f * sp.s;
    if (sp.thick < 1.0f) sp.thick = 1.0f;

    switch (id)
    {
    case IconId::General:
    {
        // M4 10.6 L12 4 L20 10.6 V19 a1.4 -> 18.6 20.4 h-3.4 v-5.2 H8.8 v5.2 H5.4 a1.4 -> 4 19 z
        Arc(dl, sp, 18.6f, 19.0f, 1.4f, 0.0f, kPi * 0.5f, col);              // 右上圆角
        Arc(dl, sp, 5.4f, 19.0f, 1.4f, kPi * 0.5f, kPi, col);                // 左下圆角
        const float roof[] = { 4, 10.6f, 12, 4, 20, 10.6f };
        Poly(dl, sp, roof, 3, col, false);
        const float body[] = { 20, 10.6f, 20, 19, 18.6f, 20.4f };
        Poly(dl, sp, body, 3, col, false);
        const float door[] = { 18.6f, 20.4f, 15.2f, 20.4f, 15.2f, 15.2f,
                               8.8f, 15.2f, 8.8f, 20.4f, 5.4f, 20.4f };
        Poly(dl, sp, door, 6, col, false);
        const float left[] = { 5.4f, 20.4f, 4, 19, 4, 10.6f };
        Poly(dl, sp, left, 3, col, false);
        break;
    }
    case IconId::Look:
    {
        d2d::StrokeCircle(dl, sp.P(12, 12), 7.6f * sp.s, col, sp.thick);
        ImU32 half = WithAlpha(col, AlphaOf(col) * 0.55f);
        dl->PathArcTo(sp.P(12, 12), 7.6f * sp.s, -kPi * 0.5f, kPi * 0.5f, 24);
        dl->PathFillConvex(half);
        break;
    }
    case IconId::Motion:
    {
        const float bolt[] = { 13.4f, 3.2f, 5.8f, 13.4f, 11.1f, 13.4f,
                               10.0f, 20.8f, 18.0f, 10.2f, 12.6f, 10.2f };
        Poly(dl, sp, bolt, 6, col, true);
        break;
    }
    case IconId::Layout:
    {
        const float r = 1.6f * sp.s;
        const float cells[4][2] = { { 4, 4 }, { 13, 4 }, { 4, 13 }, { 13, 13 } };
        for (int i = 0; i < 4; ++i)
        {
            ImVec2 a = sp.P(cells[i][0], cells[i][1]);
            ImVec2 b = sp.P(cells[i][0] + 7.0f, cells[i][1] + 7.0f);
            dl->AddRect(a, b, col, r, 0, sp.thick);
        }
        break;
    }
    case IconId::Type:
    {
        const float t1[] = { 5, 19, 5, 7, 11, 7 };
        Poly(dl, sp, t1, 3, col, false);
        const float t2[] = { 8, 7, 8, 19 };
        Poly(dl, sp, t2, 2, col, false);
        const float a1[] = { 15, 19, 18, 7, 21, 19 };
        Poly(dl, sp, a1, 3, col, false);
        const float a2[] = { 16.2f, 15, 19.8f, 15 };
        Poly(dl, sp, a2, 2, col, false);
        break;
    }
    case IconId::Data:
    {
        const float bars[] = { 4, 19, 4, 9,    9.3f, 19, 9.3f, 5,
                               14.7f, 19, 14.7f, 12,  20, 19, 20, 15 };
        for (int i = 0; i < 4; ++i)
            Poly(dl, sp, &bars[i * 4], 2, col, false);
        break;
    }
    case IconId::Keys:
    {
        ImVec2 a = sp.P(3, 6);
        ImVec2 b = sp.P(21, 18);
        dl->AddRect(a, b, col, 2.6f * sp.s, 0, sp.thick);
        const float keys[] = { 6.5f, 10, 7.5f, 10,  9.5f, 10, 10.5f, 10,
                               12.5f, 10, 13.5f, 10, 15.5f, 10, 17.5f, 10 };
        for (int i = 0; i < 4; ++i)
            Poly(dl, sp, &keys[i * 4], 2, col, false);
        const float space[] = { 8, 14, 16, 14 };
        Poly(dl, sp, space, 2, col, false);
        break;
    }
    case IconId::About:
    {
        d2d::StrokeCircle(dl, sp.P(12, 12), 8.4f * sp.s, col, sp.thick);
        const float stem[] = { 12, 11, 12, 16.6f };
        Poly(dl, sp, stem, 2, col, false);
        dl->AddCircleFilled(sp.P(12, 7.8f), sp.thick * 0.5f, col, 8);
        break;
    }
    case IconId::Theme:
    {
        // 外弧：圆心 (12,12) r8.4，16.7° -> 253.3°（绕过下方与左侧）
        // 内弧：圆心 (12.23,11.77) r8.2，17.9° -> -108°（绕过右上方）
        ImVec2 pts[52];
        int n = 0;
        const int seg = 22;
        for (int i = 0; i <= seg; ++i)
        {
            float a = 0.2915f + (4.4213f - 0.2915f) * ((float)i / seg);
            pts[n++] = sp.P(12.0f + cosf(a) * 8.4f, 12.0f + sinf(a) * 8.4f);
        }
        for (int i = 0; i <= seg; ++i)
        {
            float a = -1.8849f + (0.3124f - (-1.8849f)) * ((float)i / seg);
            pts[n++] = sp.P(12.23f + cosf(a) * 8.2f, 11.77f + sinf(a) * 8.2f);
        }
        d2d::StrokePolyline(dl, pts, n, col, sp.thick, true);
        break;
    }
    }
}

} // namespace panel
