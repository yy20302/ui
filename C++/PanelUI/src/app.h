// ============================================================================
//  app.h — 窗口 / D3D11 / 主循环
// ============================================================================
#pragma once

#include <windows.h>
#include <d3d11.h>

#include "imgui.h"
#include "theme.h"
#include "fonts.h"
#include "state.h"
#include "snow.h"
#include "backdrop.h"
#include "panel.h"

namespace panel {

class App
{
public:
    bool Init(HINSTANCE instance);
    int  Run();
    void Shutdown();

    // 供 WndProc 使用
    float Dpi() const { return dpi_; }
    bool  Ready() const { return ready_; }
    ImVec2 DragMin() const { return dragMin_; }
    ImVec2 DragMax() const { return dragMax_; }

private:
    bool CreateDevice();
    void DestroyDevice();
    void CreateRenderTarget();
    void DestroyRenderTarget();
    void RenderFrame();
    void ApplyWindowStyle();
    void LoadWindowPlacement();
    void SaveWindowPlacement();
    void PollSystemTheme();

    HINSTANCE    instance_ = nullptr;
    HWND         hwnd_     = nullptr;
    ID3D11Device*        device_  = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain*      swap_    = nullptr;
    ID3D11RenderTargetView* rtv_  = nullptr;
    bool  swapOccluded_ = false;
    float dpi_ = 1.0f;

    UiState   state_;
    Fonts     fonts_;
    Snow      snow_;
    Backdrop  backdrop_;
    Theme     theme_;

    ImVec2 dragMin_ = ImVec2(0, 0);
    ImVec2 dragMax_ = ImVec2(0, 0);

    int   themePoll_ = 0;
    bool  ready_ = false;
    bool  quit_ = false;
    double startTime_ = 0.0;

    // 窗口位置记忆
    bool rememberPos_ = false;
    int  savedX_ = 0, savedY_ = 0, savedW_ = 0, savedH_ = 0;
};

} // namespace panel
