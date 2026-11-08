// ============================================================================
//  fonts.h — 字体装载
//  ImGui 1.92 起支持按需光栅化任意字号，所以这里只装三种字形来源，
//  绘制时用 dl->AddText(font, size, ...) 指定像素大小即可。
// ============================================================================
#pragma once

#include "imgui.h"

namespace panel {

struct Fonts
{
    ImFont* ui     = nullptr;   // 正文：Segoe UI + 微软雅黑
    ImFont* uiBold = nullptr;   // 标题：Segoe UI Semibold + 微软雅黑 Bold
    ImFont* mono   = nullptr;   // 时钟 / 数值 / 键帽：Consolas

    bool Load();
};

} // namespace panel
