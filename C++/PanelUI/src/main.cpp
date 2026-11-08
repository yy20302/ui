// ============================================================================
//  main.cpp — 程序入口
// ============================================================================
#include <windows.h>

#include "app.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
    panel::App app;

    if (!app.Init(instance))
    {
        MessageBoxW(nullptr,
                    L"初始化失败：无法创建窗口或 Direct3D 11 设备。\n"
                    L"请确认显卡驱动正常，并且系统为 Windows 10 及以上。",
                    L"Panel UI", MB_OK | MB_ICONERROR);
        return 1;
    }

    int code = app.Run();
    app.Shutdown();
    return code;
}
