// ============================================================================
//  backdrop.cpp
// ============================================================================
#include "backdrop.h"

#include <d3dcompiler.h>
#include <cstring>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

namespace panel {

namespace {

// 顶点着色器用一个覆盖全屏的三角形，不需要顶点缓冲
// 像素着色器三套：壁纸 / 降采样 / 可分离高斯
const char* kShaderSrc = R"HLSL(
cbuffer Params : register(b0)
{
    float2 gRes;      // 目标分辨率
    float2 gTexel;    // 源纹理 1/size
    float  gSigma;    // 模糊 sigma（源纹理像素）
    float  gSat;      // 饱和度
    float2 gDir;      // 模糊方向
    float  gScale;    // DPI 缩放
    float3 gPad;
};

Texture2D    gTex : register(t0);
SamplerState gSmp : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

VSOut VSMain(uint id : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((id << 1) & 2, id & 2);
    o.uv  = uv;
    o.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

float3 Lerp3(float3 a, float3 b, float t) { return a + (b - a) * t; }

// 单个径向光斑：中心满色，到 fade 处透明
float SpotAlpha(float2 p, float2 center, float2 radii, float fade)
{
    float2 d = (p - center) / max(radii, float2(1.0, 1.0));
    return saturate(1.0 - length(d) / fade);
}

float4 PSWallpaper(VSOut i) : SV_TARGET
{
    float2 sz  = gRes;
    float2 p   = i.uv * sz;
    float  sc  = max(gScale, 0.5);

    // 基底 linear-gradient(158deg, #0a0d12, #06080c 52%, #040508)
    float  ang = radians(158.0);
    float2 dir = float2(sin(ang), -cos(ang));
    float2 c   = sz * 0.5;
    float  rng = abs(sz.x * dir.x) + abs(sz.y * dir.y);
    float  t   = saturate(dot(p - c, dir) / max(rng, 1.0) + 0.5);

    float3 col = Lerp3(float3(0.0392, 0.0510, 0.0706),
                       float3(0.0235, 0.0314, 0.0471), saturate(t / 0.52));
    col = Lerp3(col, float3(0.0157, 0.0196, 0.0314), saturate((t - 0.52) / 0.48));

    // 四层径向渐变，CSS 里先写的在上层，所以倒序叠加
    float a4 = SpotAlpha(p, sz * float2(0.10, 1.08), float2(800, 520) * sc, 0.58);
    col = lerp(col, float3(0.137, 0.133, 0.196), a4);

    float a3 = SpotAlpha(p, sz * float2(0.72, 1.18), float2(1000, 700) * sc, 0.60);
    col = lerp(col, float3(0.0627, 0.1020, 0.1490), a3);

    float a2 = SpotAlpha(p, sz * float2(0.86, -0.06), float2(900, 560) * sc, 0.56);
    col = lerp(col, float3(0.3020, 0.2270, 0.1330), a2);

    float a1 = SpotAlpha(p, sz * float2(0.16, -0.12), float2(1100, 620) * sc, 0.60);
    col = lerp(col, float3(0.1686, 0.2353, 0.3216), a1);

    return float4(col, 1.0);
}

float4 PSDown(VSOut i) : SV_TARGET
{
    float2 o = gTexel * 0.5;
    float4 c = gTex.Sample(gSmp, i.uv + float2(-o.x, -o.y))
             + gTex.Sample(gSmp, i.uv + float2( o.x, -o.y))
             + gTex.Sample(gSmp, i.uv + float2(-o.x,  o.y))
             + gTex.Sample(gSmp, i.uv + float2( o.x,  o.y));
    return c * 0.25;
}

float4 PSBlur(VSOut i) : SV_TARGET
{
    float sigma = max(gSigma, 0.35);
    float4 acc  = float4(0, 0, 0, 0);
    float  wsum = 0.0;

    [unroll]
    for (int k = -8; k <= 8; ++k)
    {
        float w = exp(-0.5 * (k * k) / (sigma * sigma));
        acc  += gTex.Sample(gSmp, i.uv + gTexel * gDir * k) * w;
        wsum += w;
    }

    float4 c = acc / wsum;
    float  lum = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
    c.rgb = lerp(float3(lum, lum, lum), c.rgb, gSat);
    return c;
}
)HLSL";

struct CBuffer
{
    float res[2];
    float texel[2];
    float sigma;
    float sat;
    float dir[2];
    float scale;
    float pad[3];
};

ID3DBlob* CompileShader(const char* entry, const char* target)
{
    ID3DBlob* code = nullptr;
    ID3DBlob* err  = nullptr;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
    HRESULT hr = D3DCompile(kShaderSrc, strlen(kShaderSrc), "panel_backdrop.hlsl", nullptr,
                            nullptr, entry, target, flags, 0, &code, &err);
    if (FAILED(hr))
    {
        if (err) OutputDebugStringA((const char*)err->GetBufferPointer());
        if (err) err->Release();
        if (code) code->Release();
        return nullptr;
    }
    if (err) err->Release();
    return code;
}

bool MakeRenderTarget(ID3D11Device* dev, int w, int h, ID3D11Texture2D** tex,
                      ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width  = (UINT)w;
    desc.Height = (UINT)h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(dev->CreateTexture2D(&desc, nullptr, tex))) return false;
    if (rtv && FAILED(dev->CreateRenderTargetView(*tex, nullptr, rtv))) return false;
    if (srv && FAILED(dev->CreateShaderResourceView(*tex, nullptr, srv))) return false;
    return true;
}

} // namespace

bool Backdrop::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    device_  = device;
    context_ = context;
    return CompileShaders() && CreateStates() && CreateNoise();
}

bool Backdrop::CompileShaders()
{
    ID3DBlob* vsBlob = CompileShader("VSMain", "vs_5_0");
    if (!vsBlob) return false;
    HRESULT hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                             nullptr, &vs_);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    struct { const char* entry; ID3D11PixelShader** out; } passes[] = {
        { "PSWallpaper", &psWall_ },
        { "PSDown",      &psDown_ },
        { "PSBlur",      &psBlur_ },
    };
    for (int i = 0; i < 3; ++i)
    {
        ID3DBlob* blob = CompileShader(passes[i].entry, "ps_5_0");
        if (!blob) return false;
        hr = device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                        passes[i].out);
        blob->Release();
        if (FAILED(hr)) return false;
    }
    return true;
}

bool Backdrop::CreateStates()
{
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth      = sizeof(CBuffer);
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device_->CreateBuffer(&bd, nullptr, &cb_))) return false;

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD   = D3D11_FLOAT32_MAX;
    if (FAILED(device_->CreateSamplerState(&sd, &sampler_))) return false;

    // 全屏三角形不关心朝向，直接关掉剔除
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rs = nullptr;
    if (FAILED(device_->CreateRasterizerState(&rd, &rs))) return false;
    context_->RSSetState(rs);
    rs->Release();

    return true;
}

bool Backdrop::CreateNoise()
{
    // ImGui 的采样器是 CLAMP，纹理不能被平铺，所以直接生成一张
    // 够大的白噪声，整块贴在面板上（0..1 UV）
    const int N = 512;
    unsigned char* data = new unsigned char[N * N * 4];
    for (int y = 0; y < N; ++y)
    {
        for (int x = 0; x < N; ++x)
        {
            unsigned int h = (unsigned int)(x * 374761393u + y * 668265263u);
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= h >> 16;
            unsigned char v = (unsigned char)(88 + (h & 0x4F));   // 88..175 的细颗粒
            int i = (y * N + x) * 4;
            data[i + 0] = v;
            data[i + 1] = v;
            data[i + 2] = v;
            data[i + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width  = N;
    desc.Height = N;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = data;
    init.SysMemPitch = N * 4;

    HRESULT hr = device_->CreateTexture2D(&desc, &init, &noiseTex_);
    delete[] data;
    if (FAILED(hr)) return false;
    if (FAILED(device_->CreateShaderResourceView(noiseTex_, nullptr, &noiseSRV_))) return false;
    return true;
}

void Backdrop::ReleaseTargets()
{
    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(wallTex_); rel(wallRTV_); rel(wallSRV_);
    rel(halfTex_); rel(halfRTV_); rel(halfSRV_);
    rel(aTex_);    rel(aRTV_);    rel(aSRV_);
    rel(bTex_);    rel(bRTV_);    rel(bSRV_);
    blurSRV_ = nullptr;
}

bool Backdrop::CreateTargets(int w, int h)
{
    ReleaseTargets();
    if (w < 4) w = 4;
    if (h < 4) h = 4;

    width_  = w;
    height_ = h;
    qw_     = w / 4; if (qw_ < 2) qw_ = 2;
    qh_     = h / 4; if (qh_ < 2) qh_ = 2;

    if (!MakeRenderTarget(device_, w, h, &wallTex_, &wallRTV_, &wallSRV_)) return false;
    if (!MakeRenderTarget(device_, qw_, qh_, &halfTex_, &halfRTV_, &halfSRV_)) return false;
    if (!MakeRenderTarget(device_, qw_, qh_, &aTex_, &aRTV_, &aSRV_)) return false;
    if (!MakeRenderTarget(device_, qw_, qh_, &bTex_, &bRTV_, &bSRV_)) return false;
    blurSRV_ = bSRV_;
    return true;
}

void Backdrop::Resize(int w, int h)
{
    if (w == width_ && h == height_) return;
    CreateTargets(w, h);
}

void Backdrop::BindTarget(ID3D11RenderTargetView* rtv, int w, int h)
{
    ID3D11ShaderResourceView* none = nullptr;
    context_->PSSetShaderResources(0, 1, &none);
    context_->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width  = (float)w;
    vp.Height = (float)h;
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);
}

void Backdrop::DrawFullscreen(ID3D11PixelShader* ps, ID3D11ShaderResourceView* srv,
                              float sigma, float dirX, float dirY, float sat)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(context_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        CBuffer* cb = (CBuffer*)mapped.pData;
        cb->res[0]  = (float)qw_;
        cb->res[1]  = (float)qh_;
        cb->texel[0] = 1.0f / (float)qw_;
        cb->texel[1] = 1.0f / (float)qh_;
        cb->sigma   = sigma;
        cb->sat     = sat;
        cb->dir[0]  = dirX;
        cb->dir[1]  = dirY;
        cb->scale   = 1.0f;
        cb->pad[0] = cb->pad[1] = cb->pad[2] = 0.0f;
        context_->Unmap(cb_, 0);
    }

    context_->PSSetShader(ps, nullptr, 0);
    context_->PSSetShaderResources(0, 1, &srv);
    context_->PSSetSamplers(0, 1, &sampler_);
    context_->Draw(3, 0);
}

void Backdrop::Render(float blurRadius, float saturation)
{
    if (!device_ || !vs_ || width_ < 4) return;

    if (saturation <= 0.0f) saturation = 1.0f;

    ID3D11ShaderResourceView* none = nullptr;
    context_->PSSetShaderResources(0, 1, &none);
    context_->OMSetRenderTargets(1, &wallRTV_, nullptr);
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vs_, nullptr, 0);
    context_->VSSetConstantBuffers(0, 1, &cb_);
    context_->PSSetConstantBuffers(0, 1, &cb_);
    context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    D3D11_VIEWPORT vp = {};
    vp.Width  = (float)width_;
    vp.Height = (float)height_;
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);

    // ---- 1. 壁纸 ----
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(context_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CBuffer* cb = (CBuffer*)mapped.pData;
            cb->res[0]   = (float)width_;
            cb->res[1]   = (float)height_;
            cb->texel[0] = 1.0f / (float)width_;
            cb->texel[1] = 1.0f / (float)height_;
            cb->sigma    = 1.0f;
            cb->sat      = 1.0f;
            cb->dir[0]   = 0.0f;
            cb->dir[1]   = 0.0f;
            cb->scale    = 1.0f;
            cb->pad[0] = cb->pad[1] = cb->pad[2] = 0.0f;
            context_->Unmap(cb_, 0);
        }
        context_->PSSetShader(psWall_, nullptr, 0);
        context_->PSSetShaderResources(0, 1, &wallSRV_);
        context_->PSSetSamplers(0, 1, &sampler_);
        context_->Draw(3, 0);
    }

    // ---- 2. 降采样到 1/4 ----
    {
        BindTarget(halfRTV_, qw_, qh_);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(context_->Map(cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CBuffer* cb = (CBuffer*)mapped.pData;
            cb->res[0]   = (float)qw_;
            cb->res[1]   = (float)qh_;
            cb->texel[0] = 1.0f / (float)width_;   // 源纹理是全分辨率
            cb->texel[1] = 1.0f / (float)height_;
            cb->sigma    = 1.0f;
            cb->sat      = 1.0f;
            cb->dir[0]   = 0.0f;
            cb->dir[1]   = 0.0f;
            cb->scale    = 1.0f;
            cb->pad[0] = cb->pad[1] = cb->pad[2] = 0.0f;
            context_->Unmap(cb_, 0);
        }
        context_->PSSetShader(psDown_, nullptr, 0);
        context_->PSSetShaderResources(0, 1, &wallSRV_);
        context_->PSSetSamplers(0, 1, &sampler_);
        context_->Draw(3, 0);
    }

    // ---- 3. 可分离高斯：迭代次数由模糊半径换算 ----
    float sigmaQ = blurRadius * 0.25f;              // 1/4 分辨率下的 sigma
    float perPass = sigmaQ;
    int   iters = 1;
    if (perPass > 4.0f)
    {
        perPass = 4.0f;
        iters = (int)ceilf((sigmaQ * sigmaQ) / (perPass * perPass));
        if (iters > 12) iters = 12;
    }
    if (perPass < 0.4f) perPass = 0.4f;

    // 饱和度只作用在最后一遍，否则会随迭代次数被反复放大
    ID3D11ShaderResourceView* src = halfSRV_;
    for (int i = 0; i < iters; ++i)
    {
        bool last = (i == iters - 1);
        // 横向：src -> a
        BindTarget(aRTV_, qw_, qh_);
        DrawFullscreen(psBlur_, src, perPass, 1.0f, 0.0f, 1.0f);
        // 纵向：a -> b
        BindTarget(bRTV_, qw_, qh_);
        DrawFullscreen(psBlur_, aSRV_, perPass, 0.0f, 1.0f, last ? saturation : 1.0f);
        src = bSRV_;
    }
    blurSRV_ = bSRV_;

    // 解绑，避免 ImGui 采样时冲突
    context_->PSSetShaderResources(0, 1, &none);
}

void Backdrop::Shutdown()
{
    ReleaseTargets();

    auto rel = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(vs_); rel(psWall_); rel(psDown_); rel(psBlur_);
    rel(layout_); rel(cb_); rel(sampler_);
    rel(noiseTex_); rel(noiseSRV_);
}

} // namespace panel
