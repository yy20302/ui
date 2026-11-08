// ============================================================================
//  panel.cpp — 界面绘制
//
//  布局与视觉全部照 ui-panel.html 的 CSS 还原：
//    左侧图标栏 46px / 面板 830x560 / 圆角 22 / 内边距 30 / 行高自适应
//  所有尺寸先按 CSS 像素计算，再乘以 (DPI x 界面缩放)。
// ============================================================================
#include "panel.h"
#include "draw2d.h"
#include "widgets.h"
#include "icons.h"
#include "sections.h"

#include <cmath>
#include <cstdio>
#include <ctime>

namespace panel {

namespace {

using namespace panel::d2d;

enum SectionIdx
{
    S_GENERAL = 0,
    S_LOOK,
    S_MOTION,
    S_LAYOUT,
    S_TYPE,
    S_DATA,
    S_KEYS,
    S_ABOUT
};

struct UI
{
    FrameCtx*    c  = nullptr;
    ImDrawList*  dl = nullptr;
    const Theme* th = nullptr;
    const Fonts* fn = nullptr;
    UiState*     st = nullptr;

    float  d  = 1.0f;    // DPI
    float  s  = 1.0f;    // 面板内部缩放（DPI x 界面缩放）
    float  ga = 1.0f;    // 全局入场透明度
    ImVec2 scr;

    ImVec2 pMin, pMax;   // 面板
    ImVec2 rMin, rMax;   // 图标栏
    float  radius   = 22.0f;
    float  padX     = 30.0f;
    float  headH    = 0.0f;
    float  footH    = 0.0f;
    float  bodyTop  = 0.0f;
    float  bodyBot  = 0.0f;
    float  bodyH    = 0.0f;
    float  rowH     = 48.0f;
    float  rowsTop  = 0.0f;
    bool   railHoriz = false;

    ImVec2 railBtnMin[10];
    ImVec2 railBtnMax[10];
    int    railBtnCount = 0;
    int    hoverRail    = -1;

    bool requestQuit = false;

    ImU32 A(ImU32 col, float k = 1.0f) const { return Fade(col, ga * k); }

    void Run();
    void Layout();
    void DrawWallpaper();
    void DrawPanel();
    void DrawRail();
    void DrawHead();
    void DrawBody();
    void DrawFoot();
    void DrawRipples();
    void DrawTooltip();
    void HandleKeys();
    void SelectSection(int idx);
    void Row(int si, int ri, float top);
};

// ------------------------------------------------------------------ 布局
void UI::Layout()
{
    d   = c->dpi;
    s   = d * (st->uiScale / 100.0f);
    scr = c->screen;

    float cssW = scr.x / d;
    float cssH = scr.y / d;

    float pw = cssW - 140.0f;
    if (pw > 830.0f) pw = 830.0f;
    if (pw < 320.0f) pw = cssW - 40.0f;
    if (pw < 200.0f) pw = 200.0f;

    float ph = cssH - 56.0f;
    if (ph > 560.0f) ph = 560.0f;
    if (ph < 260.0f) ph = cssH - 20.0f;
    if (ph < 160.0f) ph = 160.0f;

    float panelW = pw * s;
    float panelH = ph * s;

    railHoriz = st->rows[S_LAYOUT][2].b;

    if (!railHoriz)
    {
        pMin = ImVec2((scr.x - panelW) * 0.5f, (scr.y - panelH) * 0.5f);
        pMax = ImVec2(pMin.x + panelW, pMin.y + panelH);
        float rw = 46.0f * d;
        float rx = 18.0f * d;
        rMin = ImVec2(rx, pMin.y);
        rMax = ImVec2(rx + rw, pMax.y);
    }
    else
    {
        float rh   = 46.0f * d;
        float gap  = 12.0f * d;
        float top  = (scr.y - (rh + gap + panelH)) * 0.5f;
        pMin = ImVec2((scr.x - panelW) * 0.5f, top + rh + gap);
        pMax = ImVec2(pMin.x + panelW, pMin.y + panelH);
        rMin = ImVec2(pMin.x, top);
        rMax = ImVec2(pMax.x, top + rh);
    }

    radius = st->radius * s;
    padX   = st->rowPad * s;

    float eS   = 10.0f * s;
    float tS   = 24.0f * s;
    float subS = 12.0f * s;
    headH = 24.0f * s + eS * 1.34f + 7.0f * s + tS * 1.34f + 5.0f * s + subS * 1.34f + 18.0f * s;

    float fS = st->rows[S_DATA][5].f * s;
    footH = 12.0f * s + fS * 1.34f + 12.0f * s + 1.0f;

    bodyTop = pMin.y + headH;
    bodyBot = pMax.y - footH;
    bodyH   = bodyBot - bodyTop;
    if (bodyH < 40.0f) bodyH = 40.0f;

    bool  compact = st->rows[S_LAYOUT][0].b;
    float rowSlider = st->rows[S_LAYOUT][3].f;
    float density = (compact ? 0.80f : 1.0f) * (rowSlider / 44.0f);
    float avail = (bodyH - 12.0f * s) / 6.0f;
    rowH = avail * density;
    if (rowH < 34.0f * s) rowH = 34.0f * s;
    if (rowH > avail)     rowH = avail;

    rowsTop = bodyTop + 6.0f * s + (bodyH - 12.0f * s - 6.0f * rowH) * 0.5f;

    // 入场：整体上浮 18px + 淡入
    bool animate = st->rows[S_MOTION][0].b && !st->reduceMotion;
    float rise = 1.0f;
    if (animate)
    {
        float t = (float)(c->now - st->appStart) / 0.8f;
        rise = EaseRise(t);
    }
    ga = rise;

    float dy = (1.0f - rise) * 18.0f * s;
    if (dy > 0.01f)
    {
        pMin.y += dy; pMax.y += dy;
        rMin.y += dy; rMax.y += dy;
        bodyTop += dy; bodyBot += dy; rowsTop += dy;
    }
}

// ------------------------------------------------------------------ 壁纸
void UI::DrawWallpaper()
{
    if (!c->wallpaper) return;
    dl->AddImage(c->wallpaper, ImVec2(0, 0), scr);
}

// ------------------------------------------------------------------ 面板
void UI::DrawPanel()
{
    // 外投影
    SoftShadow(dl, pMin, pMax, radius, A(Rgba(0, 0, 0, 0.55f)), 60.0f * s, 9);

    bool glass = st->rows[S_GENERAL][0].b;

    if (glass && c->blurTex)
    {
        ImVec2 uv0(pMin.x / scr.x, pMin.y / scr.y);
        ImVec2 uv1(pMax.x / scr.x, pMax.y / scr.y);
        dl->AddImageRounded(c->blurTex, pMin, pMax, uv0, uv1, IM_COL32_WHITE, radius);
    }
    else
    {
        FillRoundRect(dl, pMin, pMax, radius, A(Rgba(9, 11, 15, 255)));
    }

    // 玻璃渐变：以主题默认值为基准，由「面板不透明度」等比缩放
    float base1 = th->night ? 0.54f : 0.87f;          // CSS 里 night / light 的 --ga1
    float ratio = th->night ? 0.815f : 0.759f;        // --ga2 / --ga1
    float a1 = base1 * (st->glassOpacity / 58.0f);
    if (a1 > 1.0f) a1 = 1.0f;
    ImU32 gt = WithAlpha(th->glassTop, a1 * ga);
    ImU32 gb = WithAlpha(th->glassBot, a1 * ratio * ga);
    FillRoundRectVGradient(dl, pMin, pMax, radius, gt, gb);

    // 顶部高光
    if (st->rows[S_LOOK][0].b)
        TopInnerHighlight(dl, pMin, pMax, radius, A(Rgba(255, 255, 255, 0.075f)), 220.0f * s);

    // 细颗粒噪点
    if (st->rows[S_LOOK][3].b && c->noiseTex)
    {
        ImU32 col = th->night ? Rgba(255, 255, 255, 0.038f * ga) : Rgba(40, 46, 54, 0.03f * ga);
        dl->PushClipRect(pMin, pMax, true);
        dl->AddImage(c->noiseTex, pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), col);
        dl->PopClipRect();
    }

    // 描边
    StrokeRoundRect(dl, pMin, pMax, radius, A(th->glassLine), 1.0f);

    // 雪花
    if (st->snowOn && c->snow)
    {
        dl->PushClipRect(pMin, pMax, true);
        c->snow->Draw(dl, pMin, radius, Fade(th->snow, ga));
        dl->PopClipRect();
    }
}

// ------------------------------------------------------------------ 图标栏
void UI::DrawRail()
{
    float k  = d;
    float rr = railHoriz ? 18.0f * k : 23.0f * k;

    SoftShadow(dl, rMin, rMax, rr, A(Rgba(0, 0, 0, 0.50f)), 34.0f * k, 8);
    FillRoundRectVGradient(dl, rMin, rMax, rr, A(th->railTop), A(th->railBot));
    StrokeRoundRect(dl, rMin, rMax, rr, A(th->railLine), 1.0f);

    const int nBtn = kSectionCount;          // 8
    const int total = nBtn + 2;              // 8 按钮 + 分隔线 + 主题按钮
    float btnS = 34.0f * k;
    float pad  = (railHoriz ? 8.0f : 10.0f) * k;
    float sepT = 1.0f * k;

    ImVec2 pos[10];
    ImVec2 size[10];
    int n = 0;

    if (!railHoriz)
    {
        float innerH = (rMax.y - rMin.y) - pad * 2.0f;
        float items  = nBtn * btnS + sepT + btnS;
        float gap    = (innerH - items) / (float)(total - 1);
        if (gap < 0.0f) gap = 0.0f;
        float y = rMin.y + pad;
        float cx = (rMin.x + rMax.x) * 0.5f;
        for (int i = 0; i < nBtn; ++i)
        {
            size[n] = ImVec2(btnS, btnS);
            pos[n]  = ImVec2(cx - btnS * 0.5f, y);
            y += btnS + gap;
            ++n;
        }
        size[n] = ImVec2(22.0f * k, sepT);
        pos[n]  = ImVec2(cx - 11.0f * k, y + (gap - sepT) * 0.5f);
        y += sepT + gap;
        ++n;
        size[n] = ImVec2(btnS, btnS);
        pos[n]  = ImVec2(cx - btnS * 0.5f, y);
        ++n;
    }
    else
    {
        float gap = 1.0f * k;
        float items = nBtn * btnS + (nBtn - 1) * gap + 8.0f * k + btnS;
        float x = rMin.x + ((rMax.x - rMin.x) - items) * 0.5f;
        float cy = (rMin.y + rMax.y) * 0.5f;
        for (int i = 0; i < nBtn; ++i)
        {
            size[n] = ImVec2(btnS, btnS);
            pos[n]  = ImVec2(x, cy - btnS * 0.5f);
            x += btnS + gap;
            ++n;
        }
        x += 8.0f * k;
        size[n] = ImVec2(btnS, btnS);
        pos[n]  = ImVec2(x, cy - btnS * 0.5f);
        ++n;
    }

    railBtnCount = n;

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    for (int i = 0; i < n; ++i)
    {
        bool isSep   = (!railHoriz && i == nBtn);
        bool isTheme = (i == n - 1);
        bool active  = (!isTheme && !isSep && i == st->section);

        if (isSep)
        {
            dl->AddRectFilledMultiColor(pos[i], ImVec2(pos[i].x + size[i].x, pos[i].y + size[i].y),
                                        A(WithAlpha(th->hair, 0.0f)),
                                        A(WithAlpha(th->hair, 0.0f)),
                                        A(th->hair), A(th->hair));
            // 用渐变线近似（左右透明、中间实色）
            dl->AddRectFilledMultiColor(pos[i], ImVec2(pos[i].x + size[i].x, pos[i].y + size[i].y),
                                        A(Rgba(255, 255, 255, 0.0f)),
                                        A(Rgba(255, 255, 255, 0.0f)),
                                        A(th->hair), A(th->hair));
            railBtnMin[i] = pos[i];
            railBtnMax[i] = ImVec2(pos[i].x + size[i].x, pos[i].y + size[i].y);
            continue;
        }

        ImVec2 mn = pos[i];
        ImVec2 mx = ImVec2(pos[i].x + size[i].x, pos[i].y + size[i].y);
        railBtnMin[i] = mn;
        railBtnMax[i] = mx;

        char id[24];
        snprintf(id, sizeof(id), "##rail%d", i);
        ImGui::SetCursorScreenPos(mn);
        ImGui::InvisibleButton(id, size[i]);
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

        float& hv = st->railHover[i];
        hv = Approach(hv, (hovered && !active) ? 1.0f : 0.0f, 14.0f, dt);

        if (hovered && !railHoriz) hoverRail = i;

        // activePop 动画
        float scale = 1.0f;
        if (st->railPop[i] > 0.0f)
        {
            float p = 1.0f - st->railPop[i];
            if (p < 0.6f) scale = 0.86f + 0.20f * EaseSpring(p / 0.6f);
            else          scale = 1.06f - 0.06f * ((p - 0.6f) / 0.4f);
            st->railPop[i] -= dt / 0.45f;
            if (st->railPop[i] < 0.0f) st->railPop[i] = 0.0f;
        }

        ImVec2 c0((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
        float bw = size[i].x * scale;
        float bh = size[i].y * scale;
        ImVec2 bmn(c0.x - bw * 0.5f, c0.y - bh * 0.5f);
        ImVec2 bmx(c0.x + bw * 0.5f, c0.y + bh * 0.5f);

        if (active)
        {
            FillRoundRect(dl, bmn, bmx, 10.0f * k, A(th->railActive));
        }
        else if (hv > 0.01f)
        {
            FillRoundRect(dl, bmn, bmx, 10.0f * k,
                          A(WithAlpha(th->railHover, AlphaOf(th->railHover) * hv)));
        }

        // 左侧指示条
        if (active && !railHoriz)
        {
            float bx = bmn.x - 9.0f * k;
            if (bx < rMin.x + 3.0f * k) bx = rMin.x + 3.0f * k;
            ImVec2 b0(bx, c0.y - 8.0f * k);
            ImVec2 b1(bx + 3.0f * k, c0.y + 8.0f * k);
            SoftBlob(dl, c0, 14.0f * k, A(Rgba(190, 205, 225, 0.30f)), 8);
            FillRoundRectVGradient(dl, b0, b1, 2.0f * k, A(th->accentA), A(th->accentB));
        }
        else if (active && railHoriz)
        {
            ImVec2 b0(c0.x - 7.0f * k, bmx.y - 3.0f * k - 6.0f * k);
            ImVec2 b1(c0.x + 7.0f * k, bmx.y - 6.0f * k);
            FillRoundRect(dl, b0, b1, 1.5f * k, A(th->accentA));
        }

        IconId icon = isTheme ? IconId::Theme : kSections[i].icon;
        ImU32 iconCol = active ? A(th->ink) : A(LerpColor(th->railIcon, th->ink, hv));
        DrawIcon(dl, icon, c0, 17.0f * k * scale, iconCol);

        if (clicked)
        {
            st->SpawnRipple(io.MousePos, mn, mx);
            if (isTheme) st->night = !st->night;
            else         SelectSection(i);
        }
    }
}

void UI::SelectSection(int idx)
{
    if (idx < 0 || idx >= kSectionCount) return;
    st->section  = idx;
    st->sectionT = c->now;
    st->railPop[idx] = 1.0f;
}

// ------------------------------------------------------------------ 头部
void UI::DrawHead()
{
    const SectionDef& sec = kSections[st->section];
    float left = pMin.x + padX;
    float y = pMin.y + 24.0f * s;

    // 小标题
    float eS = 10.0f * s;
    bool upper = st->rows[S_TYPE][1].b;
    const char* eyebrow = upper ? "PANEL" : "Panel";
    TextSpaced(dl, fn->ui, eS, ImVec2(left, y), A(th->ink3), eyebrow, 0.34f * eS);
    y += eS * 1.34f + 7.0f * s;

    // 标题
    float tS = 24.0f * s;
    float spacing = st->rows[S_TYPE][2].b ? 0.05f * tS : 0.0f;
    if (spacing > 0.0f)
        TextSpaced(dl, fn->uiBold, tS, ImVec2(left, y), A(th->ink), sec.title, spacing);
    else
        Text(dl, fn->uiBold, tS, ImVec2(left, y), A(th->ink), sec.title);
    y += tS * 1.34f + 5.0f * s;

    // 副标题
    Text(dl, fn->ui, 12.0f * s, ImVec2(left, y), A(th->ink2), sec.sub);

    // ---- 右上角（从右往左排：指示灯 / 序号 / 时钟 / 版本）----
    float right  = pMax.x - padX;
    float metaCy = pMin.y + 24.0f * s + 4.0f * s + 12.0f * s;

    // 呼吸指示灯（最右）
    if (st->rows[S_DATA][1].b)
    {
        float period = st->rows[S_DATA][4].f;
        if (period < 0.5f) period = 0.5f;
        float ph = (float)fmod((double)c->now, (double)period) / period;
        float pulse = 0.5f - 0.5f * cosf(ph * 6.2831853f);

        ImVec2 dc(right - 3.5f * s, metaCy);
        SoftBlob(dl, dc, (7.0f + 5.0f * pulse) * s, A(Rgba(150, 165, 185, 0.45f)), 10);
        dl->AddCircleFilled(dc, 3.5f * s, A(th->accentB), 24);
        right -= 7.0f * s + 9.0f * s;
    }

    // 分组序号
    if (st->rows[S_DATA][3].b)
    {
        char idx[16];
        snprintf(idx, sizeof(idx), "%02d / %02d", st->section + 1, kSectionCount);
        float fs = 11.0f * s;
        ImVec2 ts = Measure(fn->mono, fs, idx);
        float w = ts.x + 18.0f * s;
        float h = fs * 1.34f + 8.0f * s;
        ImVec2 mn(right - w, metaCy - h * 0.5f), mx(right, metaCy + h * 0.5f);
        FillRoundRect(dl, mn, mx, 7.0f * s, A(th->chipBg));
        StrokeRoundRect(dl, mn, mx, 7.0f * s, A(th->hair), 1.0f);
        TextCentered(dl, fn->mono, fs, mn, mx, A(th->ink2), idx);
        right -= w + 9.0f * s;
    }

    // 时钟
    {
        char clock[16];
        time_t tt = time(nullptr);
        struct tm lt;
        localtime_s(&lt, &tt);
        snprintf(clock, sizeof(clock), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);

        float fs = 12.0f * s;
        ImFont* f = st->rows[S_TYPE][0].b ? fn->mono : fn->ui;
        ImVec2 ts = Measure(f, fs, clock);
        float w = ts.x + 20.0f * s;
        float h = fs * 1.34f + 8.0f * s;
        ImVec2 mn(right - w, metaCy - h * 0.5f), mx(right, metaCy + h * 0.5f);
        FillRoundRect(dl, mn, mx, 7.0f * s, A(th->chipBg));
        StrokeRoundRect(dl, mn, mx, 7.0f * s, A(th->hair), 1.0f);
        TextCentered(dl, f, fs, mn, mx, A(th->ink), clock);
        right -= w + 9.0f * s;
    }

    // 版本号（最左）
    if (st->rows[S_DATA][0].b)
    {
        float fs = 11.0f * s;
        ImVec2 ts = Measure(fn->mono, fs, "v0.3");
        float w = ts.x + 18.0f * s;
        float h = fs * 1.34f + 8.0f * s;
        ImVec2 mn(right - w, metaCy - h * 0.5f), mx(right, metaCy + h * 0.5f);
        FillRoundRect(dl, mn, mx, 7.0f * s, A(th->chipBg));
        StrokeRoundRect(dl, mn, mx, 7.0f * s, A(th->hair), 1.0f);
        TextCentered(dl, fn->mono, fs, mn, mx, A(th->ink2), "v0.3");
        right -= w + 9.0f * s;
    }

    // 分隔线
    dl->AddLine(ImVec2(pMin.x, pMin.y + headH), ImVec2(pMax.x, pMin.y + headH),
                A(th->hair), 1.0f);
}

// ------------------------------------------------------------------ 行
void UI::Row(int si, int ri, float top)
{
    const RowDef& def = kSections[si].rows[ri];
    RowRuntime&   rt  = st->rows[si][ri];

    bool trans = st->rows[S_MOTION][0].b && !st->reduceMotion;
    float ra = 1.0f;
    if (trans)
    {
        float t = (float)((c->now - st->sectionT) - (0.02 + 0.04 * ri)) / 0.5f;
        ra = EaseRise(t);
    }
    float dy = (1.0f - ra) * 8.0f * s;

    ImVec2 mn(pMin.x, top + dy);
    ImVec2 mx(pMax.x, top + rowH + dy);
    float cy = (mn.y + mx.y) * 0.5f;

    // 悬停底色
    bool hovered = ImGui::IsMouseHoveringRect(mn, mx);
    rt.hover = Approach(rt.hover, hovered ? 1.0f : 0.0f, 14.0f, c->dt);
    if (rt.hover > 0.01f)
    {
        float k = st->rows[S_MOTION][5].f / 45.0f;
        float a = AlphaOf(th->rowHover) * rt.hover * k;
        FillRoundRect(dl, mn, mx, 0.0f, A(WithAlpha(th->rowHover, a)));
    }

    // 分隔线
    if (ri > 0 && st->rows[S_LAYOUT][1].b)
        dl->AddLine(ImVec2(pMin.x + padX, top), ImVec2(pMax.x - padX, top), A(th->hair), 1.0f);

    // 文字
    float nameS = st->nameSize * s;
    float descS = 12.0f * s;
    float gap   = st->lineGap * s;
    ImFont* nf  = (st->nameWeight >= 600.0f) ? fn->uiBold : fn->ui;
    float nameH = nameS * 1.34f;
    float descH = descS * 1.34f;
    float blockH = nameH + gap + descH;
    float ty = cy - blockH * 0.5f;

    ImVec2 namePos(pMin.x + padX, ty);
    Text(dl, nf, nameS, namePos, A(th->ink, ra), def.name);
    if (st->nameWeight >= 700.0f)
        Text(dl, nf, nameS, ImVec2(namePos.x + 0.55f, namePos.y), A(th->ink, ra), def.name);

    Text(dl, fn->ui, descS, ImVec2(pMin.x + padX, ty + nameH + gap), A(th->ink3, ra), def.desc);

    // ---- 控件 ----
    float ctrlRight = pMax.x - padX;

    if (def.kind == RowKind::Toggle)
    {
        float sw = 54.0f * s, sh = 30.0f * s;
        ImVec2 a(ctrlRight - sw, cy - sh * 0.5f);
        ImVec2 b(ctrlRight, cy + sh * 0.5f);

        bool on = (def.ref != Ref::None) ? st->RefBool(def.ref, si, ri) : rt.b;
        char id[32];
        snprintf(id, sizeof(id), "##sw%d_%d", si, ri);

        bool hv = false, held = false;
        bool clicked = wg::SwitchInput(id, a, b, hv, held);
        if (clicked)
        {
            on = !on;
            if (def.ref != Ref::None) st->SetRefBool(def.ref, on);
            else                      rt.b = on;
            st->SpawnRipple(ImGui::GetIO().MousePos, a, b);
        }
        rt.press = Approach(rt.press, held ? 1.0f : 0.0f, 26.0f, c->dt);
        wg::SwitchDraw(dl, a, b, s, on, rt.knob, rt.knobVel, rt.press, *th, hv,
                       st->rows[S_MOTION][1].b);
    }
    else if (def.kind == RowKind::Slider)
    {
        float ctrlW = 330.0f * s;
        float maxW  = (pMax.x - padX) - (pMin.x + padX) - 180.0f * s;
        if (ctrlW > maxW) ctrlW = maxW;
        if (ctrlW < 160.0f * s) ctrlW = 160.0f * s;

        ImVec2 a(ctrlRight - ctrlW, cy - 13.0f * s);
        ImVec2 b(ctrlRight, cy + 13.0f * s);

        float v = (def.ref != Ref::None) ? st->RefFloat(def.ref, si, ri) : rt.f;
        char id[32];
        snprintf(id, sizeof(id), "##sl%d_%d", si, ri);

        bool hv = false, act = false;
        if (wg::SliderInput(id, a, b, s, def.min, def.max, def.step, &v, rt.pop, hv, act))
        {
            if (def.ref != Ref::None) st->SetRefFloat(def.ref, v);
            else                      rt.f = v;
        }
        rt.f = v;

        char label[32];
        FormatValue(def, v, label, sizeof(label));
        wg::SliderDraw(dl, a, b, s, *th, fn->mono, def.min, def.max, v, label, rt.pop, hv, act);
    }
    else if (def.kind == RowKind::Key)
    {
        float x = ctrlRight;
        for (int i = def.keyCount - 1; i >= 0; --i)
        {
            ImVec2 sz = wg::KbdSize(s, fn->mono, def.keys[i]);
            ImVec2 a(x - sz.x, cy - sz.y * 0.5f);
            ImVec2 b(x, cy + sz.y * 0.5f);
            wg::Kbd(dl, a, b, s, *th, fn->mono, def.keys[i]);
            x -= sz.x + 5.0f * s;
        }
    }
    else if (def.kind == RowKind::Meta)
    {
        float fs = 12.5f * s;
        ImVec2 ts = Measure(fn->ui, fs, def.meta);
        Text(dl, fn->ui, fs, ImVec2(ctrlRight - ts.x, cy - ts.y * 0.5f), A(th->ink2, ra),
             def.meta);
    }
}

void UI::DrawBody()
{
    for (int ri = 0; ri < 6; ++ri)
        Row(st->section, ri, rowsTop + rowH * ri);
}

// ------------------------------------------------------------------ 页脚
void UI::DrawFoot()
{
    float top = pMax.y - footH;
    dl->AddRectFilled(ImVec2(pMin.x, top), pMax, A(th->footBg), radius,
                      ImDrawFlags_RoundCornersBottom);
    dl->AddLine(ImVec2(pMin.x, top), ImVec2(pMax.x, top), A(th->hair), 1.0f);

    float fS = st->rows[S_DATA][5].f * s;
    float cy = top + footH * 0.5f;
    float left  = pMin.x + padX;
    float right = pMax.x - padX;

    ImVec2 m1 = Measure(fn->mono, fS, "UI ONLY");
    Text(dl, fn->mono, fS, ImVec2(left, cy - m1.y * 0.5f), A(th->ink2), "UI ONLY");

    const char* tail = " · 不含任何功能逻辑";
    Text(dl, fn->ui, fS, ImVec2(left + m1.x, cy - m1.y * 0.5f), A(th->ink3), tail);

    if (st->rows[S_DATA][2].b)
    {
        const char* hint = "点击左侧图标切换分组";
        ImVec2 m2 = Measure(fn->ui, fS, hint);
        Text(dl, fn->ui, fS, ImVec2(right - m2.x, cy - m2.y * 0.5f), A(th->ink3), hint);
    }
}

// ------------------------------------------------------------------ 波纹
void UI::DrawRipples()
{
    for (int i = 0; i < 8; ++i)
    {
        Ripple& rp = st->ripples[i];
        if (!rp.used) continue;
        float t = ((float)c->now - rp.start) / 0.62f;
        if (t < 0.0f || t > 1.0f) { rp.used = false; continue; }
        dl->PushClipRect(rp.mn, rp.mx, true);
        wg::Ripple(dl, rp.pos, rp.mn, rp.mx, t, *th);
        dl->PopClipRect();
    }
}

// ------------------------------------------------------------------ 提示
void UI::DrawTooltip()
{
    if (hoverRail < 0 || hoverRail >= railBtnCount || railHoriz) return;
    if (hoverRail == kSectionCount) return;   // 分隔线

    const char* text = (hoverRail == railBtnCount - 1) ? "切换主题" : kSections[hoverRail].label;
    float k = d;
    float hv = st->railHover[hoverRail];
    if (hv < 0.02f) return;

    float fs = 11.5f * k;
    ImVec2 ts = Measure(fn->ui, fs, text);
    float w = ts.x + 18.0f * k;
    float h = fs * 1.34f + 10.0f * k;

    float x = railBtnMax[hoverRail].x + 12.0f * k - 4.0f * k * (1.0f - hv);
    float cy = (railBtnMin[hoverRail].y + railBtnMax[hoverRail].y) * 0.5f;
    ImVec2 mn(x, cy - h * 0.5f);
    ImVec2 mx(x + w, cy + h * 0.5f);

    ImU32 bg = th->night ? Rgba(26, 28, 34, 0.94f) : Rgba(250, 250, 252, 0.96f);
    SoftShadow(dl, mn, mx, 8.0f * k, A(Rgba(0, 0, 0, 0.45f * hv)), 12.0f * k, 5);
    FillRoundRect(dl, mn, mx, 8.0f * k, A(Fade(bg, hv)));
    StrokeRoundRect(dl, mn, mx, 8.0f * k, A(Fade(th->railLine, hv)), 1.0f);
    TextCentered(dl, fn->ui, fs, mn, mx, A(Fade(th->ink, hv)), text);
}

// ------------------------------------------------------------------ 快捷键
void UI::HandleKeys()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && io.KeyShift)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_T, false))
        {
            st->night = !st->night;
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            st->snowOn = !st->snowOn;
            st->rows[S_LOOK][2].b = st->snowOn;
            st->rows[S_LOOK][2].knob = st->snowOn ? 1.0f : 0.0f;
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R, false))
        {
            st->glassOpacity = 58.0f;
            st->blurRadius   = 54.0f;
            st->rowPad       = 30.0f;
            st->radius       = 22.0f;
            st->nameSize     = 15.0f;
            st->nameWeight   = 400.0f;
            st->lineGap      = 3.0f;
            st->uiScale      = 100.0f;
            st->snowDensity  = 45.0f;
            st->snowSpeed    = 18.0f;
            for (int si = 0; si < kSectionCount; ++si)
                for (int ri = 0; ri < 6; ++ri)
                {
                    const RowDef& df = kSections[si].rows[ri];
                    st->rows[si][ri].f = df.f;
                    st->rows[si][ri].b = df.b;
                    st->rows[si][ri].knob = df.b ? 1.0f : 0.0f;
                }
            return;
        }
    }

    if (!st->rows[S_KEYS][0].b) return;

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) ||
        ImGui::IsKeyPressed(ImGuiKey_Tab, false))
    {
        SelectSection((st->section + 1) % kSectionCount);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
    {
        SelectSection((st->section + kSectionCount - 1) % kSectionCount);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        requestQuit = true;
    }
}

// ------------------------------------------------------------------ 主流程
void UI::Run()
{
    dl = c->dl;
    th = &c->theme;
    fn = &c->fonts;
    st = c->st;

    HandleKeys();
    Layout();

    // 雪花：尺寸 / 密度变化时重建，每帧推进
    if (c->snow)
    {
        c->snow->Resize(pMax.x - pMin.x, pMax.y - pMin.y);
        c->snow->SetDensity(st->snowDensity);
        c->snow->Update(c->dt, st->snowSpeed, st->reduceMotion || !st->snowOn);
    }

    // 拖动热区：面板头部
    c->dragMin = ImVec2(pMin.x, pMin.y);
    c->dragMax = ImVec2(pMax.x, pMin.y + headH);

    DrawWallpaper();
    DrawPanel();
    DrawRail();
    DrawHead();
    DrawBody();
    DrawFoot();
    DrawRipples();
    DrawTooltip();
}

} // namespace

bool DrawFrame(FrameCtx& ctx)
{
    UI ui;
    ui.c = &ctx;
    ui.Run();
    return ui.requestQuit;
}

} // namespace panel
