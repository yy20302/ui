// ============================================================================
//  icons.h — 左侧图标栏的矢量图标
//  原 HTML 用内联 SVG（24x24，stroke 1.6，round cap/join），这里用 ImGui
//  的路径描边逐条还原，缩放后依然锐利。
// ============================================================================
#pragma once

#include "imgui.h"

namespace panel {

enum class IconId
{
    General,   // 房子
    Look,      // 半明半暗的圆
    Motion,    // 闪电
    Layout,    // 四宫格
    Type,      // T + A
    Data,      // 柱状图
    Keys,      // 键盘
    About,     // 信息圆
    Theme      // 月牙
};

// center 为图标中心，size 为 24x24 视口缩放后的边长
void DrawIcon(ImDrawList* dl, IconId id, ImVec2 center, float size, ImU32 col);

} // namespace panel
