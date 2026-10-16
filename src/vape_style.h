// Vape V4 palette + color helpers.
// Mirrors src/guis/new/libraries/{uipallet,color}.lua from VapeV4ForRoblox.
#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace vape {

// --- Base palette (uipallet.lua) ---
// Main = RGB(26, 25, 26), Text = RGB(200, 200, 200)
inline ImVec4 MainColor()  { return ImVec4(26.0f / 255.0f, 25.0f / 255.0f, 26.0f / 255.0f, 1.0f); }
inline ImVec4 TextColor()  { return ImVec4(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f); }
inline ImU32  StrokeColor(){ return IM_COL32(85, 85, 85, 51); } // transparency 0.8

// --- Layout constants (from GUI.lua / Category.lua / Module.lua) ---
inline constexpr float kPanelW   = 220.0f; // reference window width (GUI.lua/Category.lua: 220)
inline constexpr float kHeaderH  = 41.0f;  // collapsed category window height
inline constexpr float kRowsTop  = 37.0f;  // reference: Children frame + divider sit at y=37
inline constexpr float kModuleH  = 40.0f;
inline constexpr float kToggleH  = 30.0f;
inline constexpr float kSliderH  = 50.0f;
inline constexpr float kRadius   = 8.0f;
// Rows run full-bleed to the panel edge, so the row list has to stop at least a
// radius above the bottom: that is where the rounded silhouette starts curving
// inward, and anything drawn below it pokes outside the panel outline.
inline constexpr float kBotGap   = kRadius;
inline constexpr float kMaxOpenH  = 560.0f; // expanded category clamp (601 in lua incl. header)
// Floating search bar (SearchBar.lua): detached bar pinned to the top centre.
inline constexpr float kSearchH    = 37.0f; // bar frame height
inline constexpr float kSearchTopY = 13.0f; // pinned distance from the top edge
// Default top line shared by the main window, the category row and friends:
// 16px below the floating search bar.
inline constexpr float kRowTopY    = kSearchTopY + kSearchH + 16.0f; // = 66
// Legit Modules GUI (LegitWindow.lua): standalone centred window.
inline constexpr float kLegitW = 700.0f;
inline constexpr float kLegitH = 380.0f;

// --- Accent (vape.GUIColor) ---
struct Accent {
    float hue = 0.46f; // default from base.lua
    float sat = 0.96f;
    float val = 0.52f;
    bool rainbow = false;
    float rainbowSpeed = 1.0f; // cycles per ~10s
};

// The overlay window is colorkeyed: pure black is see-through, so a UI colour
// must never land on exact black or it punches a hole through the panel.
// 1.5/255 is still visually black, and stays above 1/255 after ToU32's
// truncating conversion so the byte survives at 1.
inline ImVec4 ColorkeySafe(ImVec4 c) {
    const float floorV = 1.5f / 255.0f;
    c.x = std::max(c.x, floorV);
    c.y = std::max(c.y, floorV);
    c.z = std::max(c.z, floorV);
    return c;
}

inline ImVec4 AccentRGB(const Accent& a, float timeSec = -1.0f) {
    float h = a.hue, s = a.sat, v = a.val;
    if (a.rainbow && timeSec >= 0.0f) {
        h = fmodf(timeSec * 0.1f * a.rainbowSpeed, 1.0f);
    }
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
    return ColorkeySafe(ImVec4(r, g, b, 1.0f));
}

// --- color.lua helpers ---
// Dark(col, n): V - n (main is dark); Light(col, n): V + n.
inline void RGBtoHSV(ImVec4 c, float& h, float& s, float& v) {
    ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
}
inline ImVec4 HSVtoRGB(float h, float s, float v, float a = 1.0f) {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
    return ImVec4(r, g, b, a);
}
inline ImVec4 Darken(ImVec4 c, float n) {
    float h, s, v; RGBtoHSV(c, h, s, v);
    // lua checks Main brightness; Main is dark so dark = v - n
    v = std::clamp(v - n, 0.0f, 1.0f);
    return HSVtoRGB(h, s, v, c.w);
}
inline ImVec4 Lighten(ImVec4 c, float n) {
    float h, s, v; RGBtoHSV(c, h, s, v);
    v = std::clamp(v + n, 0.0f, 1.0f);
    return HSVtoRGB(h, s, v, c.w);
}

// vape:Color(h) saturation curve (used for rainbow mode)
inline void VapeRainbow(float h, float& outS, float& outV) {
    float s = 0.74f + 0.26f * std::min(h / 0.045f, 1.0f);
    if (h > 0.577f) s = 1.0f - 0.48f * std::min((h - 0.577f) / 0.088f, 1.0f);
    if (h > 0.674f) s = 0.52f + 0.48f * std::min((h - 0.674f) / 0.149f, 1.0f);
    if (h > 0.869f) s = 1.0f - 0.26f * std::min((h - 0.869f) / 0.131f, 1.0f);
    outS = s; outV = 1.0f;
}

// vape:TextColor(h, s, v): dark text on bright accents, else white.
inline ImVec4 AccentText(float h, float s, float v) {
    if (v >= 0.7f && (s < 0.6f || (h > 0.04f && h < 0.56f)))
        return ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
    return ImVec4(1, 1, 1, 1);
}

// Soft drop shadow approximation (UIShadow BlurRadius 13, Transparency 0.25)
inline void DrawShadow(ImDrawList* dl, ImVec2 min, ImVec2 max, float rounding) {
    // 3 layered rects, decreasing alpha
    const int steps = 3;
    for (int i = steps; i >= 1; --i) {
        float pad = (float)(i * 4);
        ImU32 col = IM_COL32(0, 0, 0, (i == 3 ? 18 : (i == 2 ? 26 : 34)));
        dl->AddRectFilled(ImVec2(min.x - pad, min.y - pad),
                          ImVec2(max.x + pad, max.y + pad), col, rounding + pad);
    }
}

} // namespace vape
