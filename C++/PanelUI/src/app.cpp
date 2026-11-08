// ============================================================================
//  app.cpp — 无边框窗口 + D3D11 + Dear ImGui 主循环
// ============================================================================
#include "app.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <windowsx.h>

#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <cstdio>
#include <cstring>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

// 高版本 SDK 才有，缺失时自己补上
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWA_COLOR_NONE
#define DWMWA_COLOR_NONE 0xFFFFFFFE
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

namespace panel {

namespace {

App* g_app = nullptr;

const ImGuiWindowFlags kRootFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
    ImGuiWindowFlags_NoBackground;

std::wstring PlacementPath()
{
    wchar_t buf[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
    std::wstring dir = (n > 0) ? std::wstring(buf, n) : std::wstring(L".");
    dir += L"\\PanelUI";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\window.ini";
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &p);

        RECT rc;
        GetClientRect(hWnd, &rc);
        float dpi = g_app ? g_app->Dpi() : 1.0f;
        int   b   = (int)(7.0f * dpi);

        bool left   = p.x < b;
        bool right  = p.x >= rc.right - b;
        bool top    = p.y < b;
        bool bottom = p.y >= rc.bottom - b;

        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;

        // 面板头部可拖动窗口（对应 CSS 的 -webkit-app-region:drag）
        if (g_app)
        {
            ImVec2 a = g_app->DragMin();
            ImVec2 bmax = g_app->DragMax();
            if (bmax.x > a.x && p.x >= a.x && p.x <= bmax.x && p.y >= a.y && p.y <= bmax.y)
                return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_SIZE:
        // 注意：CreateWindow 期间就会收到 WM_SIZE，此时 ImGui 上下文还不存在
        if (wParam != SIZE_MINIMIZED && g_app && g_app->Ready())
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            PostMessageW(hWnd, WM_APP + 1, 0, 0);
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;   // 屏蔽 Alt 菜单
        break;

    case WM_DPICHANGED:
    {
        RECT* r = (RECT*)lParam;
        SetWindowPos(hWnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace

// ---------------------------------------------------------------------------
bool App::Init(HINSTANCE instance)
{
    instance_ = instance;
    g_app = this;

    ImGui_ImplWin32_EnableDpiAwareness();

    // 读取上次窗口位置
    LoadWindowPlacement();

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = instance;
    wc.hCursor       = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"PanelUIWindow";
    wc.hIcon         = LoadIconW(instance, MAKEINTRESOURCEW(101));
    wc.hIconSm       = wc.hIcon;
    if (!RegisterClassExW(&wc)) return false;

    // 初始尺寸按 CSS 设计稿 1010x700，再乘 DPI
    HDC screenDc = GetDC(nullptr);
    float dpi = (float)GetDeviceCaps(screenDc, LOGPIXELSX);
    ReleaseDC(nullptr, screenDc);
    if (dpi < 96.0f) dpi = 96.0f;
    dpi_ = dpi / 96.0f;

    int w = (int)(1010.0f * dpi_);
    int h = (int)(700.0f * dpi_);

    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int x = wa.left + ((wa.right - wa.left) - w) / 2;
    int y = wa.top + ((wa.bottom - wa.top) - h) / 2;

    if (rememberPos_)
    {
        if (savedW_ > 200 && savedH_ > 200) { w = savedW_; h = savedH_; }
        x = savedX_;
        y = savedY_;
    }

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, L"PanelUIWindow", L"Panel UI",
                            WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN, x, y, w, h,
                            nullptr, nullptr, instance, nullptr);
    if (!hwnd_) return false;

    ApplyWindowStyle();

    HDC hdc = GetDC(hwnd_);
    dpi_ = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(hwnd_, hdc);
    if (dpi_ < 0.5f) dpi_ = 1.0f;

    if (!CreateDevice()) return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    // ---- ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowPadding    = ImVec2(0, 0);
    style.AntiAliasedLines = true;
    style.AntiAliasedFill  = true;

    if (!fonts_.Load()) return false;

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(device_, context_);

    if (!backdrop_.Init(device_, context_)) return false;

    startTime_ = ImGui::GetTime();
    state_.Init(startTime_);
    theme_ = MakeNightTheme();

    // 跟随系统主题时先取一次
    themePoll_ = 100;
    PollSystemTheme();
    ready_ = true;

    return true;
}

void App::ApplyWindowStyle()
{
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    int corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // 去掉系统绘制的边框，只保留阴影
    COLORREF none = (COLORREF)DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(hwnd_, DWMWA_BORDER_COLOR, &none, sizeof(none));

    // 深色标题栏（无边框时用于阴影配色）
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hwnd_, &margins);
}

// ---------------------------------------------------------------------------
bool App::CreateDevice()
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount        = 2;
    sd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow       = hwnd_;
    sd.SampleDesc.Count   = 1;
    sd.Windowed           = TRUE;
    sd.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, 2, D3D11_SDK_VERSION, &sd,
        &swap_, &device_, &level, &context_);

    if (FAILED(hr))
    {
        // 有些环境没有 D3D_DEBUG，退回普通模式
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                           D3D11_SDK_VERSION, &sd, &swap_, &device_, &level,
                                           &context_);
    }
    if (FAILED(hr)) return false;

    CreateRenderTarget();
    return true;
}

void App::CreateRenderTarget()
{
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(swap_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back)) && back)
    {
        device_->CreateRenderTargetView(back, nullptr, &rtv_);
        back->Release();
    }
}

void App::DestroyRenderTarget()
{
    if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
}

void App::DestroyDevice()
{
    DestroyRenderTarget();
    if (swap_)    { swap_->Release();    swap_ = nullptr; }
    if (context_) { context_->Release(); context_ = nullptr; }
    if (device_)  { device_->Release();  device_ = nullptr; }
}

// ---------------------------------------------------------------------------
void App::PollSystemTheme()
{
    if (!state_.rows[0][1].b) return;      // 跟随系统主题未开启
    if (--themePoll_ > 0) return;
    themePoll_ = 60;

    DWORD light = 1;
    DWORD size  = sizeof(light);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &key) == ERROR_SUCCESS)
    {
        RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&light, &size);
        RegCloseKey(key);
    }
    state_.night = (light == 0);
}

void App::LoadWindowPlacement()
{
    std::wstring path = PlacementPath();
    wchar_t buf[64] = {};

    GetPrivateProfileStringW(L"window", L"x", L"", buf, 64, path.c_str());
    if (buf[0]) savedX_ = _wtoi(buf);
    GetPrivateProfileStringW(L"window", L"y", L"", buf, 64, path.c_str());
    if (buf[0]) savedY_ = _wtoi(buf);
    GetPrivateProfileStringW(L"window", L"w", L"", buf, 64, path.c_str());
    if (buf[0]) savedW_ = _wtoi(buf);
    GetPrivateProfileStringW(L"window", L"h", L"", buf, 64, path.c_str());
    if (buf[0]) savedH_ = _wtoi(buf);
    GetPrivateProfileStringW(L"window", L"valid", L"0", buf, 64, path.c_str());
    rememberPos_ = (buf[0] == L'1');
}

void App::SaveWindowPlacement()
{
    if (!hwnd_) return;
    if (!state_.rows[0][3].b) return;      // 记住窗口位置

    RECT rc;
    if (!GetWindowRect(hwnd_, &rc)) return;

    std::wstring path = PlacementPath();
    wchar_t buf[64];
    swprintf_s(buf, L"%ld", rc.left);   WritePrivateProfileStringW(L"window", L"x", buf, path.c_str());
    swprintf_s(buf, L"%ld", rc.top);    WritePrivateProfileStringW(L"window", L"y", buf, path.c_str());
    swprintf_s(buf, L"%ld", rc.right - rc.left);  WritePrivateProfileStringW(L"window", L"w", buf, path.c_str());
    swprintf_s(buf, L"%ld", rc.bottom - rc.top);  WritePrivateProfileStringW(L"window", L"h", buf, path.c_str());
    WritePrivateProfileStringW(L"window", L"valid", L"1", path.c_str());
}

// ---------------------------------------------------------------------------
void App::RenderFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (state_.night != theme_.night)
        theme_ = state_.night ? MakeNightTheme() : MakeLightTheme();

    // ---- 壁纸 + 背板模糊 ----
    int w = (int)io.DisplaySize.x;
    int h = (int)io.DisplaySize.y;
    if (w > 0 && h > 0)
    {
        backdrop_.Resize(w, h);
        float sat = state_.rows[1][1].b ? 1.25f : 1.0f;   // 彩色折射
        backdrop_.Render(state_.blurRadius, sat);
    }

    // ---- 界面 ----
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##root", nullptr, kRootFlags);

    FrameCtx fc;
    fc.dl        = ImGui::GetWindowDrawList();
    fc.theme     = theme_;
    fc.fonts     = fonts_;
    fc.st        = &state_;
    fc.snow      = &snow_;
    fc.dpi       = dpi_;
    fc.screen    = io.DisplaySize;
    fc.dt        = io.DeltaTime;
    fc.now       = ImGui::GetTime();
    fc.wallpaper = backdrop_.Wallpaper();
    fc.blurTex   = backdrop_.Blurred();
    fc.noiseTex  = backdrop_.Noise();

    if (DrawFrame(fc)) quit_ = true;

    dragMin_ = fc.dragMin;
    dragMax_ = fc.dragMax;

    ImGui::End();
    ImGui::PopStyleVar(2);

    // ---- 提交 ----
    ImGui::Render();

    if (rtv_)
    {
        context_->OMSetRenderTargets(1, &rtv_, nullptr);
        const float clear[4] = { 0.02f, 0.024f, 0.031f, 1.0f };
        context_->ClearRenderTargetView(rtv_, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    swapOccluded_ = (swap_->Present(1, 0) == DXGI_STATUS_OCCLUDED);
}

int App::Run()
{
    while (!quit_)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) quit_ = true;
            if (msg.message == WM_APP + 1)
            {
                // 尺寸变化：重建后备缓冲
                DestroyRenderTarget();
                if (swap_)
                {
                    swap_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                    CreateRenderTarget();
                }
            }
        }
        if (quit_) break;

        // 最小化时不渲染，避免 0 尺寸后备缓冲
        if (IsIconic(hwnd_))
        {
            Sleep(10);
            continue;
        }

        // 被遮挡时降频等待，重新可见后恢复
        if (swapOccluded_)
        {
            if (swap_->Present(0, 0) == S_OK) swapOccluded_ = false;
            else { Sleep(50); continue; }
        }

        RenderFrame();

        // 每秒检查一次系统主题
        PollSystemTheme();
    }
    return 0;
}

void App::Shutdown()
{
    SaveWindowPlacement();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    backdrop_.Shutdown();
    DestroyDevice();

    if (hwnd_) DestroyWindow(hwnd_);
    UnregisterClassW(L"PanelUIWindow", instance_);
    g_app = nullptr;
}

} // namespace panel
