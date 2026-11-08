// ============================================================================
//  sections.cpp — 分组数据（与 ui-panel.html 的 SECTIONS 完全对应）
// ============================================================================
#include "sections.h"

#include <cstdio>

namespace panel {

const SectionDef kSections[] = {
    // ------------------------------------------------------------ 01 通用
    {
        .id = "general", .label = "通用", .title = "通用", .sub = "面板基础行为",
        .icon = IconId::General,
        .rows = {
            { .kind = RowKind::Toggle, .name = "毛玻璃",       .desc = "使用背景模糊渲染面板",     .b = true  },
            { .kind = RowKind::Toggle, .name = "跟随系统主题", .desc = "自动匹配浅色 / 深色",      .b = false },
            { .kind = RowKind::Toggle, .name = "启动时居中",   .desc = "出现时把面板放到屏幕中央", .b = true  },
            { .kind = RowKind::Toggle, .name = "记住窗口位置", .desc = "下次打开回到上次的位置",   .b = true  },
            { .kind = RowKind::Slider, .name = "面板不透明度", .desc = "数值越低越透明",
              .f = 58.0f,  .min = 30.0f,  .max = 100.0f, .step = 1.0f,
              .ref = Ref::GlassOpacity, .fmt = OutFmt::Percent },
            { .kind = RowKind::Slider, .name = "界面缩放",     .desc = "整体等比放大",
              .f = 100.0f, .min = 80.0f,  .max = 130.0f, .step = 1.0f,
              .ref = Ref::UiScale, .fmt = OutFmt::Percent },
        },
    },
    // ------------------------------------------------------------ 02 外观
    {
        .id = "look", .label = "外观", .title = "外观", .sub = "玻璃质感与描边",
        .icon = IconId::Look,
        .rows = {
            { .kind = RowKind::Toggle, .name = "高光描边",   .desc = "顶边一条细腻的高光",     .b = true },
            { .kind = RowKind::Toggle, .name = "彩色折射",   .desc = "模糊时保留背景色彩",     .b = true },
            { .kind = RowKind::Toggle, .name = "雪花特效",   .desc = "面板内飘落的雪",         .b = true, .ref = Ref::SnowOn },
            { .kind = RowKind::Toggle, .name = "细颗粒噪点", .desc = "消除大面积纯色塑料感",   .b = true },
            { .kind = RowKind::Slider, .name = "雪花密度",   .desc = "单位面积内的雪花数量",
              .f = 45.0f, .min = 0.0f, .max = 100.0f, .step = 1.0f,
              .ref = Ref::SnowDensity, .fmt = OutFmt::Percent },
            { .kind = RowKind::Slider, .name = "模糊半径",   .desc = "背景虚化强度",
              .f = 54.0f, .min = 0.0f, .max = 100.0f, .step = 1.0f,
              .ref = Ref::BlurRadius, .fmt = OutFmt::Px },
        },
    },
    // ------------------------------------------------------------ 03 动效
    {
        .id = "motion", .label = "动效", .title = "动效", .sub = "过渡与飘落",
        .icon = IconId::Motion,
        .rows = {
            { .kind = RowKind::Toggle, .name = "过渡动画",     .desc = "切换分组时淡入",         .b = true  },
            { .kind = RowKind::Toggle, .name = "弹性曲线",     .desc = "开关回弹更自然",         .b = true  },
            { .kind = RowKind::Toggle, .name = "减弱动态效果", .desc = "暂停雪花与过渡动画",     .b = false, .ref = Ref::ReduceMotion },
            { .kind = RowKind::Slider, .name = "飘落速度",     .desc = "雪花下落快慢",
              .f = 18.0f,  .min = 4.0f,   .max = 40.0f,  .step = 1.0f,
              .ref = Ref::SnowSpeed, .fmt = OutFmt::PxPerSec },
            { .kind = RowKind::Slider, .name = "过渡时长",     .desc = "数值越大越缓",
              .f = 260.0f, .min = 80.0f,  .max = 800.0f, .step = 10.0f, .fmt = OutFmt::Ms },
            { .kind = RowKind::Slider, .name = "悬停高亮",     .desc = "鼠标移入行的亮度",
              .f = 45.0f,  .min = 0.0f,   .max = 100.0f, .step = 1.0f, .fmt = OutFmt::Percent },
        },
    },
    // ------------------------------------------------------------ 04 布局
    {
        .id = "layout", .label = "布局", .title = "布局", .sub = "尺寸与密度",
        .icon = IconId::Layout,
        .rows = {
            { .kind = RowKind::Toggle, .name = "紧凑模式",   .desc = "缩短行高，显示更多条目", .b = false },
            { .kind = RowKind::Toggle, .name = "显示分隔线", .desc = "行与行之间的细线",       .b = true  },
            { .kind = RowKind::Toggle, .name = "图标栏横向", .desc = "把左侧图标排到顶部",     .b = false },
            { .kind = RowKind::Slider, .name = "行高",       .desc = "每行的高度",
              .f = 44.0f, .min = 36.0f, .max = 64.0f, .step = 1.0f, .fmt = OutFmt::Px },
            { .kind = RowKind::Slider, .name = "内边距",     .desc = "内容到边缘的距离",
              .f = 30.0f, .min = 16.0f, .max = 40.0f, .step = 1.0f,
              .ref = Ref::RowPad, .fmt = OutFmt::Px },
            { .kind = RowKind::Slider, .name = "圆角",       .desc = "面板四个角的弧度",
              .f = 22.0f, .min = 0.0f,  .max = 34.0f, .step = 1.0f,
              .ref = Ref::Radius, .fmt = OutFmt::Px },
        },
    },
    // ------------------------------------------------------------ 05 字体
    {
        .id = "type", .label = "字体", .title = "字体", .sub = "排版细节",
        .icon = IconId::Type,
        .rows = {
            { .kind = RowKind::Toggle, .name = "等宽数字",   .desc = "数值抖动时不跳动",   .b = true  },
            { .kind = RowKind::Toggle, .name = "大写小标题", .desc = "分组名前使用全大写", .b = true  },
            { .kind = RowKind::Toggle, .name = "加宽字距",   .desc = "标题更松散",         .b = false },
            { .kind = RowKind::Slider, .name = "正文字号",   .desc = "条目名称大小",
              .f = 15.0f,  .min = 12.0f,  .max = 17.0f, .step = 0.5f,
              .ref = Ref::NameSize, .fmt = OutFmt::Px },
            { .kind = RowKind::Slider, .name = "字重",       .desc = "标题的粗细",
              .f = 400.0f, .min = 400.0f, .max = 800.0f, .step = 50.0f,
              .ref = Ref::NameWeight, .fmt = OutFmt::Raw },
            { .kind = RowKind::Slider, .name = "行距",       .desc = "名称与说明之间",
              .f = 3.0f,   .min = 0.0f,   .max = 12.0f, .step = 1.0f,
              .ref = Ref::LineGap, .fmt = OutFmt::Px },
        },
    },
    // ------------------------------------------------------------ 06 显示项
    {
        .id = "data", .label = "显示项", .title = "显示项", .sub = "面板上的辅助信息",
        .icon = IconId::Data,
        .rows = {
            { .kind = RowKind::Toggle, .name = "版本号",       .desc = "右上角的版本标签",     .b = true },
            { .kind = RowKind::Toggle, .name = "状态指示灯",   .desc = "呼吸动画的小圆点",     .b = true },
            { .kind = RowKind::Toggle, .name = "底部提示语",   .desc = "页脚的操作说明",       .b = true },
            { .kind = RowKind::Toggle, .name = "分组序号",     .desc = "右上角的 01 / 08",     .b = true },
            { .kind = RowKind::Slider, .name = "状态刷新间隔", .desc = "指示灯呼吸周期",
              .f = 3.0f,  .min = 1.0f,  .max = 10.0f, .step = 0.5f, .fmt = OutFmt::Sec },
            { .kind = RowKind::Slider, .name = "页脚字号",     .desc = "底部说明文字大小",
              .f = 11.0f, .min = 10.0f, .max = 14.0f, .step = 0.5f, .fmt = OutFmt::Px },
        },
    },
    // ------------------------------------------------------------ 07 快捷键
    {
        .id = "keys", .label = "快捷键", .title = "快捷键", .sub = "仅作展示，未绑定",
        .icon = IconId::Keys,
        .rows = {
            { .kind = RowKind::Toggle, .name = "启用快捷键", .desc = "允许键盘操作面板", .b = true },
            { .kind = RowKind::Key, .name = "呼出面板", .desc = "显示或隐藏面板",
              .keys = { "Ctrl", "Shift", "P" }, .keyCount = 3 },
            { .kind = RowKind::Key, .name = "切换分组", .desc = "在分组之间循环",
              .keys = { "Tab" }, .keyCount = 1 },
            { .kind = RowKind::Key, .name = "切换主题", .desc = "浅色 / 深色互换",
              .keys = { "Ctrl", "Shift", "T" }, .keyCount = 3 },
            { .kind = RowKind::Key, .name = "切换雪花", .desc = "开或关飘雪效果",
              .keys = { "Ctrl", "Shift", "S" }, .keyCount = 3 },
            { .kind = RowKind::Key, .name = "重置布局", .desc = "恢复到默认尺寸",
              .keys = { "Ctrl", "Shift", "R" }, .keyCount = 3 },
        },
    },
    // ------------------------------------------------------------ 08 关于
    {
        .id = "about", .label = "关于", .title = "关于", .sub = "这是什么",
        .icon = IconId::About,
        .rows = {
            { .kind = RowKind::Meta, .name = "名称", .desc = "控制面板界面原型", .meta = "Panel UI" },
            { .kind = RowKind::Meta, .name = "版本", .desc = "当前构建",         .meta = "v0.3" },
            { .kind = RowKind::Meta, .name = "实现", .desc = "零框架零依赖",     .meta = "C++ · ImGui" },
            { .kind = RowKind::Meta, .name = "雪花", .desc = "逐帧绘制",         .meta = "DrawList" },
            { .kind = RowKind::Meta, .name = "渲染", .desc = "原生窗口",         .meta = "Direct3D 11" },
            { .kind = RowKind::Meta, .name = "说明", .desc = "开关只有视觉状态", .meta = "无功能逻辑" },
        },
    },
};

const int kSectionCount = (int)(sizeof(kSections) / sizeof(kSections[0]));

void FormatValue(const RowDef& row, float v, char* out, int outSize)
{
    switch (row.fmt)
    {
    case OutFmt::Percent:  snprintf(out, outSize, "%g%%", v);     break;
    case OutFmt::Px:       snprintf(out, outSize, "%g px", v);    break;
    case OutFmt::Ms:       snprintf(out, outSize, "%g ms", v);    break;
    case OutFmt::PxPerSec: snprintf(out, outSize, "%g px/s", v);  break;
    case OutFmt::Sec:      snprintf(out, outSize, "%.1f s", v);   break;
    case OutFmt::Raw:
    default:               snprintf(out, outSize, "%g", v);       break;
    }
}

} // namespace panel
