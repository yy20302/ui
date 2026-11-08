// ============================================================================
//  backdrop.h — 背景壁纸 + 面板背板模糊
//
//  浏览器里 panel 的玻璃靠 backdrop-filter: blur() 实现，原生窗口没有这个
//  能力，所以这里自己做一遍：
//    1) 全屏着色器画出 .stage 的多层径向渐变壁纸
//    2) 1/4 分辨率下做多次可分离高斯模糊（迭代次数由模糊半径换算）
//    3) 模糊结果作为纹理交给 ImGui，面板按圆角裁切后叠上去
//  这样「模糊半径」滑杆是真的在改画面。
// ============================================================================
#pragma once

#include <d3d11.h>
#include "imgui.h"

namespace panel {

class Backdrop
{
public:
    bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    void Resize(int width, int height);

    // 每帧：生成壁纸与模糊背板
    void Render(float blurRadius, float saturation);

    ImTextureID Wallpaper() const { return (ImTextureID)(intptr_t)wallSRV_; }
    ImTextureID Blurred()   const { return (ImTextureID)(intptr_t)blurSRV_; }
    ImTextureID Noise()     const { return (ImTextureID)(intptr_t)noiseSRV_; }

    int Width()  const { return width_; }
    int Height() const { return height_; }

private:
    bool CompileShaders();
    bool CreateStates();
    bool CreateNoise();
    bool CreateTargets(int w, int h);
    void ReleaseTargets();

    void BindTarget(ID3D11RenderTargetView* rtv, int w, int h);
    void DrawFullscreen(ID3D11PixelShader* ps, ID3D11ShaderResourceView* srv,
                        float sigma, float dirX, float dirY, float sat);

    ID3D11Device*        device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;

    ID3D11VertexShader*  vs_        = nullptr;
    ID3D11PixelShader*   psWall_    = nullptr;
    ID3D11PixelShader*   psDown_    = nullptr;
    ID3D11PixelShader*   psBlur_    = nullptr;
    ID3D11InputLayout*   layout_    = nullptr;
    ID3D11Buffer*        cb_        = nullptr;
    ID3D11SamplerState*  sampler_   = nullptr;

    // 全屏壁纸
    ID3D11Texture2D*          wallTex_ = nullptr;
    ID3D11RenderTargetView*   wallRTV_ = nullptr;
    ID3D11ShaderResourceView* wallSRV_ = nullptr;

    // 1/4 分辨率模糊链
    ID3D11Texture2D*          halfTex_ = nullptr;
    ID3D11RenderTargetView*   halfRTV_ = nullptr;
    ID3D11ShaderResourceView* halfSRV_ = nullptr;

    ID3D11Texture2D*          aTex_ = nullptr;
    ID3D11RenderTargetView*   aRTV_ = nullptr;
    ID3D11ShaderResourceView* aSRV_ = nullptr;

    ID3D11Texture2D*          bTex_ = nullptr;
    ID3D11RenderTargetView*   bRTV_ = nullptr;
    ID3D11ShaderResourceView* bSRV_ = nullptr;

    ID3D11ShaderResourceView* blurSRV_ = nullptr;   // 指向最终结果（a 或 b）

    // 噪点
    ID3D11Texture2D*          noiseTex_ = nullptr;
    ID3D11ShaderResourceView* noiseSRV_ = nullptr;

    int width_  = 0;
    int height_ = 0;
    int qw_     = 1;
    int qh_     = 1;
};

} // namespace panel
