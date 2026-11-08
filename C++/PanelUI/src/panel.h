// ============================================================================
//  panel.h — 界面绘制入口
// ============================================================================
#pragma once

#include "imgui.h"
#include "theme.h"
#include "fonts.h"
#include "state.h"
#include "snow.h"

namespace panel {

struct FrameCtx
{
    ImDrawList* dl        = nullptr;
    Theme       theme;
    Fonts       fonts;
    UiState*    st        = nullptr;
    Snow*       snow      = nullptr;
    float       dpi       = 1.0f;
    ImVec2      screen    = ImVec2(0, 0);
    float       dt        = 0.0f;
    double      now       = 0.0;
    ImTextureID wallpaper = 0;
    ImTextureID blurTex   = 0;
    ImTextureID noiseTex  = 0;

    // 输出：窗口拖动热区（屏幕坐标，供 WM_NCHITTEST 使用）
    ImVec2 dragMin = ImVec2(0, 0);
    ImVec2 dragMax = ImVec2(0, 0);
};

// 绘制一帧界面；返回 true 表示请求退出
bool DrawFrame(FrameCtx& ctx);

} // namespace panel
