// ============================================================================
//  state.h — 运行时状态
// ============================================================================
#pragma once

#include "imgui.h"
#include "sections.h"

namespace panel {

// 点击波纹
struct Ripple
{
    ImVec2 pos   = ImVec2(0, 0);
    ImVec2 mn    = ImVec2(0, 0);
    ImVec2 mx    = ImVec2(0, 0);
    float  start = -100.0f;
    bool   used  = false;
};

// 每一行的运行时数据
struct RowRuntime
{
    bool  b     = false;
    float f     = 0.0f;
    float knob  = 0.0f;    // 开关滑块位置 0..1
    float knobVel = 0.0f;  // 弹簧速度
    float hover = 0.0f;    // 行悬停底色
    float pop   = 0.0f;    // 数值胶囊弹跳
    float press = 0.0f;    // 开关按下（滑块变宽）
};

struct UiState
{
    // ---- 绑定全局状态（对应 HTML 里 state 对象） ----
    bool  snowOn        = true;
    bool  reduceMotion  = false;
    float snowDensity   = 45.0f;
    float snowSpeed     = 18.0f;
    float glassOpacity  = 58.0f;
    float blurRadius    = 54.0f;
    float rowPad        = 30.0f;
    float radius        = 22.0f;
    float nameSize      = 15.0f;
    float nameWeight    = 400.0f;
    float lineGap       = 3.0f;
    float uiScale       = 100.0f;

    // ---- 界面状态 ----
    bool   night      = true;
    int    section    = 0;
    double now        = 0.0;
    double appStart   = 0.0;
    double sectionT   = 0.0;   // 当前分组开始时间（行入场动画基准）
    bool   visible    = true;

    RowRuntime rows[8][6];
    float      railHover[9] = {};
    float      railPop[8]   = {};
    Ripple     ripples[8];

    void  Init(double t);
    float RefFloat(Ref r, int si, int ri);
    bool  RefBool(Ref r, int si, int ri);
    void  SetRefFloat(Ref r, float v);
    void  SetRefBool(Ref r, bool v);
    void  SpawnRipple(ImVec2 pos, ImVec2 mn, ImVec2 mx);
};

} // namespace panel
