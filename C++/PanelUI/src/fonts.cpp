// ============================================================================
//  fonts.cpp
// ============================================================================
#include "fonts.h"

#include <cstdio>

namespace panel {

namespace {

bool FileExists(const char* path)
{
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
    fclose(f);
    return true;
}

// 主字体 + 合并中文字体（雅黑），保证中英文都能正常显示
ImFont* AddMerged(const char* latin, const char* cjk, float size, bool bold)
{
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 1;
    cfg.PixelSnapH  = false;

    ImFont* base = nullptr;
    if (latin && FileExists(latin))
        base = ImGui::GetIO().Fonts->AddFontFromFileTTF(latin, size, &cfg);

    if (!base)
        base = ImGui::GetIO().Fonts->AddFontDefault();

    if (cjk && FileExists(cjk))
    {
        ImFontConfig merge;
        merge.MergeMode   = true;
        merge.OversampleH = 2;
        merge.OversampleV = 1;
        merge.PixelSnapH  = false;
        // 雅黑比 Segoe UI 稍高，稍微缩小让两种字形基线一致
        merge.SizePixels  = size;
        if (bold) merge.GlyphOffset.y = 0.0f;
        ImGui::GetIO().Fonts->AddFontFromFileTTF(cjk, size, &merge);
    }
    return base;
}

} // namespace

bool Fonts::Load()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const char* segoe  = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* segoeB = "C:\\Windows\\Fonts\\seguisb.ttf";
    const char* yahei  = "C:\\Windows\\Fonts\\msyh.ttc";
    const char* yaheiB = "C:\\Windows\\Fonts\\msyhbd.ttc";
    const char* consol = "C:\\Windows\\Fonts\\consola.ttf";

    ui     = AddMerged(segoe,  yahei,  16.0f, false);
    uiBold = AddMerged(segoeB, yaheiB, 24.0f, true);

    if (FileExists(consol))
    {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH  = false;
        mono = io.Fonts->AddFontFromFileTTF(consol, 12.0f, &cfg);
    }
    if (!mono) mono = ui;

    if (!ui)     return false;
    if (!uiBold) uiBold = ui;

    return true;
}

} // namespace panel
