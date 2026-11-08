// ============================================================================
//  state.cpp
// ============================================================================
#include "state.h"

namespace panel {

void UiState::Init(double t)
{
    appStart = t;
    now      = t;
    sectionT = t;

    for (int si = 0; si < kSectionCount; ++si)
    {
        for (int ri = 0; ri < 6; ++ri)
        {
            const RowDef& d = kSections[si].rows[ri];
            RowRuntime&   r = rows[si][ri];
            r.b    = d.b;
            r.f    = d.f;
            r.knob = d.b ? 1.0f : 0.0f;
            r.hover = 0.0f;
            r.pop   = 0.0f;
            r.press = 0.0f;

            // 带 ref 的行把初值同步到全局状态
            if (d.ref != Ref::None)
            {
                if (d.kind == RowKind::Toggle) SetRefBool(d.ref, d.b);
                else                           SetRefFloat(d.ref, d.f);
            }
        }
    }
    for (int i = 0; i < 9; ++i) railHover[i] = 0.0f;
    for (int i = 0; i < 8; ++i) railPop[i]   = 0.0f;
}

float UiState::RefFloat(Ref r, int si, int ri)
{
    switch (r)
    {
    case Ref::SnowDensity:  return snowDensity;
    case Ref::SnowSpeed:    return snowSpeed;
    case Ref::GlassOpacity: return glassOpacity;
    case Ref::BlurRadius:   return blurRadius;
    case Ref::RowPad:       return rowPad;
    case Ref::Radius:       return radius;
    case Ref::NameSize:     return nameSize;
    case Ref::NameWeight:   return nameWeight;
    case Ref::LineGap:      return lineGap;
    case Ref::UiScale:      return uiScale;
    default:                return rows[si][ri].f;
    }
}

bool UiState::RefBool(Ref r, int si, int ri)
{
    switch (r)
    {
    case Ref::SnowOn:       return snowOn;
    case Ref::ReduceMotion: return reduceMotion;
    default:                return rows[si][ri].b;
    }
}

void UiState::SetRefFloat(Ref r, float v)
{
    switch (r)
    {
    case Ref::SnowDensity:  snowDensity  = v; break;
    case Ref::SnowSpeed:    snowSpeed    = v; break;
    case Ref::GlassOpacity: glassOpacity = v; break;
    case Ref::BlurRadius:   blurRadius   = v; break;
    case Ref::RowPad:       rowPad       = v; break;
    case Ref::Radius:       radius       = v; break;
    case Ref::NameSize:     nameSize     = v; break;
    case Ref::NameWeight:   nameWeight   = v; break;
    case Ref::LineGap:      lineGap      = v; break;
    case Ref::UiScale:      uiScale      = v; break;
    default: break;
    }
}

void UiState::SetRefBool(Ref r, bool v)
{
    switch (r)
    {
    case Ref::SnowOn:       snowOn       = v; break;
    case Ref::ReduceMotion: reduceMotion = v; break;
    default: break;
    }
}

void UiState::SpawnRipple(ImVec2 pos, ImVec2 mn, ImVec2 mx)
{
    for (int i = 0; i < 8; ++i)
    {
        int idx = (i + 0) % 8;
        if (!ripples[idx].used || (float)now - ripples[idx].start > 0.62f)
        {
            ripples[idx].pos   = pos;
            ripples[idx].mn    = mn;
            ripples[idx].mx    = mx;
            ripples[idx].start = (float)now;
            ripples[idx].used  = true;
            return;
        }
    }
    // 全满：覆盖最旧的
    int oldest = 0;
    for (int i = 1; i < 8; ++i)
        if (ripples[i].start < ripples[oldest].start) oldest = i;
    ripples[oldest].pos   = pos;
    ripples[oldest].mn    = mn;
    ripples[oldest].mx    = mx;
    ripples[oldest].start = (float)now;
    ripples[oldest].used  = true;
}

} // namespace panel
