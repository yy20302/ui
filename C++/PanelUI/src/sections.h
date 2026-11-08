// ============================================================================
//  sections.h — 面板数据模型
//  8 个分组 × 6 行，与 ui-panel.html 里的 SECTIONS 数组一一对应。
//  行的四种类型：Toggle / Slider / Key（键帽）/ Meta（文本值）
// ============================================================================
#pragma once

#include "icons.h"

namespace panel {

enum class RowKind
{
    Toggle,
    Slider,
    Key,
    Meta
};

// 行绑定的全局状态（对应 HTML 里的 ref 字段）
enum class Ref
{
    None,
    SnowOn,
    SnowDensity,
    SnowSpeed,
    ReduceMotion,
    GlassOpacity,
    BlurRadius,
    RowPad,
    Radius,
    NameSize,
    NameWeight,
    LineGap,
    UiScale
};

enum class OutFmt
{
    Raw,
    Percent,
    Px,
    Ms,
    PxPerSec,
    Sec
};

struct RowDef
{
    RowKind     kind  = RowKind::Toggle;
    const char* name  = "";
    const char* desc  = "";
    bool        b     = false;      // Toggle 初值
    float       f     = 0.0f;       // Slider 初值
    float       min   = 0.0f;
    float       max   = 100.0f;
    float       step  = 1.0f;
    Ref         ref   = Ref::None;
    OutFmt      fmt   = OutFmt::Raw;
    const char* meta  = nullptr;    // Meta 显示值
    const char* keys[3] = { nullptr, nullptr, nullptr };
    int         keyCount = 0;
};

struct SectionDef
{
    const char* id    = "";
    const char* label = "";         // 图标提示
    const char* title = "";
    const char* sub   = "";
    IconId      icon  = IconId::General;
    RowDef      rows[6];
};

extern const SectionDef kSections[];
extern const int        kSectionCount;

// 把滑杆数值格式化成右侧胶囊里的文本
void FormatValue(const RowDef& row, float v, char* out, int outSize);

} // namespace panel
