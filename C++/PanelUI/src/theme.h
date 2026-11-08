// ============================================================================
//  theme.h — 配色主题（对应 ui-panel.html 里的 CSS 变量）
//  浅色 :root 与 night 两套主题，颜色全部集中在这里
// ============================================================================
#pragma once

#include "imgui.h"

namespace panel {

// ---------------------------------------------------------------- 颜色工具
inline ImU32 Rgba(int r, int g, int b, float a)
{
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return IM_COL32(r, g, b, (int)(a * 255.0f + 0.5f));
}

inline ImU32 HexRgba(unsigned rgb, float a)
{
    return Rgba((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, a);
}

// 只改透明度，保留色相
inline ImU32 WithAlpha(ImU32 c, float a)
{
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    ImU32 keep = c & ~IM_COL32_A_MASK;
    return keep | ((ImU32)(a * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
}

inline float AlphaOf(ImU32 c)
{
    return (float)((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
}

// 按比例衰减透明度（用于入场淡入）
inline ImU32 Fade(ImU32 c, float k)
{
    return WithAlpha(c, AlphaOf(c) * k);
}

inline ImU32 LerpColor(ImU32 a, ImU32 b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    const int ar = (a >> IM_COL32_R_SHIFT) & 0xFF, ag = (a >> IM_COL32_G_SHIFT) & 0xFF,
              ab = (a >> IM_COL32_B_SHIFT) & 0xFF, aa = (a >> IM_COL32_A_SHIFT) & 0xFF;
    const int br = (b >> IM_COL32_R_SHIFT) & 0xFF, bg = (b >> IM_COL32_G_SHIFT) & 0xFF,
              bb = (b >> IM_COL32_B_SHIFT) & 0xFF, ba = (b >> IM_COL32_A_SHIFT) & 0xFF;
    return IM_COL32((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t),
                    (int)(ab + (bb - ab) * t), (int)(aa + (ba - aa) * t));
}

// ---------------------------------------------------------------- 主题结构
struct Theme
{
    bool night = true;

    ImU32 ink      = 0;   // 主文字
    ImU32 ink2     = 0;   // 次级文字
    ImU32 ink3     = 0;   // 说明文字
    ImU32 hair     = 0;   // 细分隔线

    ImU32 glassTop = 0;   // 面板玻璃渐变（含 alpha）
    ImU32 glassBot = 0;
    ImU32 glassLine = 0;  // 面板描边

    ImU32 railTop = 0;    // 左侧图标栏渐变
    ImU32 railBot = 0;
    ImU32 railLine = 0;
    ImU32 railIcon = 0;
    ImU32 railHover = 0;
    ImU32 railActive = 0;

    ImU32 accent = 0;
    ImU32 accentInk = 0;
    ImU32 accentA = 0;    // 高光端
    ImU32 accentB = 0;    // 暗端
    ImU32 knob = 0;
    ImU32 knobOn = 0;

    ImU32 track = 0;
    ImU32 rowHover = 0;
    ImU32 chipBg = 0;
    ImU32 footBg = 0;

    ImU32 ripple = 0;     // 点击波纹
    ImU32 snow = 0;       // 雪花
};

inline Theme MakeNightTheme()
{
    Theme t;
    t.night     = true;
    t.ink       = HexRgba(0xeef1f6, 1.00f);
    t.ink2      = HexRgba(0xa2aab5, 1.00f);
    t.ink3      = HexRgba(0x6f7783, 1.00f);
    t.hair      = Rgba(255, 255, 255, 0.085f);

    t.glassTop  = Rgba(18, 20, 25, 0.54f);
    t.glassBot  = Rgba(6, 7, 10, 0.44f);
    t.glassLine = Rgba(255, 255, 255, 0.13f);

    t.railTop   = Rgba(22, 24, 29, 0.72f);
    t.railBot   = Rgba(10, 11, 14, 0.60f);
    t.railLine  = Rgba(255, 255, 255, 0.12f);
    t.railIcon  = Rgba(255, 255, 255, 0.50f);
    t.railHover = Rgba(255, 255, 255, 0.10f);
    t.railActive = Rgba(255, 255, 255, 0.13f);

    t.accent    = HexRgba(0xdfe5ee, 1.0f);
    t.accentInk = HexRgba(0xe6ebf2, 1.0f);
    t.accentA   = HexRgba(0xeef1f6, 1.0f);
    t.accentB   = HexRgba(0xc6ccd5, 1.0f);
    t.knob      = HexRgba(0xf2f5f9, 1.0f);
    t.knobOn    = HexRgba(0x14171b, 1.0f);

    t.track     = Rgba(255, 255, 255, 0.13f);
    t.rowHover  = Rgba(255, 255, 255, 0.045f);
    t.chipBg    = Rgba(255, 255, 255, 0.09f);
    t.footBg    = Rgba(0, 0, 0, 0.22f);

    t.ripple    = Rgba(255, 255, 255, 0.55f);
    t.snow      = Rgba(255, 255, 255, 1.0f);
    return t;
}

inline Theme MakeLightTheme()
{
    Theme t;
    t.night     = false;
    t.ink       = HexRgba(0x1a1c20, 1.00f);
    t.ink2      = HexRgba(0x474e57, 1.00f);
    t.ink3      = HexRgba(0x5f6771, 1.00f);
    t.hair      = Rgba(26, 28, 32, 0.075f);

    t.glassTop  = Rgba(255, 255, 255, 0.87f);
    t.glassBot  = Rgba(255, 255, 255, 0.66f);
    t.glassLine = Rgba(255, 255, 255, 0.80f);

    t.railTop   = Rgba(255, 255, 255, 0.66f);
    t.railBot   = Rgba(255, 255, 255, 0.42f);
    t.railLine  = Rgba(255, 255, 255, 0.72f);
    t.railIcon  = Rgba(26, 28, 32, 0.52f);
    t.railHover = Rgba(255, 255, 255, 0.62f);
    t.railActive = Rgba(255, 255, 255, 0.96f);

    t.accent    = HexRgba(0x2f343b, 1.0f);
    t.accentInk = HexRgba(0x2b3037, 1.0f);
    t.accentA   = HexRgba(0x565d67, 1.0f);
    t.accentB   = HexRgba(0x24282e, 1.0f);
    t.knob      = HexRgba(0xffffff, 1.0f);
    t.knobOn    = HexRgba(0xffffff, 1.0f);

    t.track     = Rgba(26, 28, 32, 0.12f);
    t.rowHover  = Rgba(255, 255, 255, 0.46f);
    t.chipBg    = Rgba(255, 255, 255, 0.66f);
    t.footBg    = Rgba(255, 255, 255, 0.26f);

    t.ripple    = Rgba(40, 46, 54, 0.32f);
    t.snow      = Rgba(38, 44, 52, 1.0f);
    return t;
}

} // namespace panel
