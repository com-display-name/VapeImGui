// Shared internals for the Vape V4 ClickGUI clone.
//
// These free helpers used to live in an anonymous namespace at the top of the
// monolithic vape_gui.cpp. They are header-only (inline) now, so every render
// translation unit runs exactly the code the single-file version ran:
// colourkey-safe rounded-panel primitives, accent/rainbow maths, bind-key
// names, and text/row measurement helpers. Read the note above kPi before
// touching any of the geometry - the window compositor makes subtle
// rasterization bugs visible as holes.
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <string>

#include "vape_gui.h"

// vape layout constants, used throughout the render code.
using vape::kHeaderH;
using vape::kModuleH;
using vape::kPanelW;
using vape::kRadius;
using vape::kSearchH;
using vape::kSearchTopY;
using vape::kLegitW;
using vape::kLegitH;

inline ImU32 ToU32(const ImVec4& c) {
    return IM_COL32((int)(c.x * 255), (int)(c.y * 255), (int)(c.z * 255), (int)(c.w * 255));
}

// Context-free clock for notifications (each overlay window owns an ImGui
// context with its own Time, and none exists yet at startup).
inline float VapeTime() {
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return (float)duration_cast<milliseconds>(steady_clock::now() - start).count() / 1000.0f;
}

// Bindable keys (Bind.lua): keyboard keys with a stable display name.
// Polled with GetAsyncKeyState so binds work while the game has focus.
inline int BindableKeyAt(int i) {
    static int keys[52];
    static bool init = false;
    if (!init) {
        init = true;
        int n = 0;
        for (int vk = 0x41; vk <= 0x5A; ++vk) keys[n++] = vk; // A-Z
        for (int vk = 0x30; vk <= 0x39; ++vk) keys[n++] = vk; // 0-9
        for (int vk = 0x70; vk <= 0x7B; ++vk) keys[n++] = vk; // F1-F12
        keys[n++] = VK_SPACE;
        keys[n++] = VK_SHIFT;
        keys[n++] = VK_CONTROL;
        keys[n++] = VK_MENU;
    }
    return (i >= 0 && i < 52) ? keys[i] : 0;
}

inline std::string BindKeyName(int vk) {
    if (vk >= 0x41 && vk <= 0x5A) return std::string(1, (char)vk);
    if (vk >= 0x30 && vk <= 0x39) return std::string(1, (char)vk);
    if (vk >= 0x70 && vk <= 0x7B) return "F" + std::to_string(vk - 0x70 + 1);
    switch (vk) {
    case VK_SPACE: return "SPACE";
    case VK_SHIFT: return "SHIFT";
    case VK_CONTROL: return "CTRL";
    case VK_MENU: return "ALT";
    default: break;
    }
    return "";
}

// Rounded corners are built from explicit convex pieces (rects + corner
// fans/annuli), never from PathStroke polylines. Two rasterization facts
// drive this (the window is colourkeyed: only pure black is see-through, so
// every dropped pixel becomes a visible hole):
//  - ImGui's non-AA PathStroke offsets segments with the approximate RSQRTSS
//    instruction and emits per-segment quads with no join geometry. A 1px
//    stroke's straight edges can land epsilon off the pixel center and be
//    dropped entirely (or double up), and the corner bands get wedge gaps at
//    every vertex.
//  - Any diagonal edge passing exactly through a pixel center is rejected by
//    both triangles sharing it (top-left rule), which punches pinholes at
//    integer/half-integer panel positions.
// So: fill and outline share the exact same corner arc math (float-identical
// vertices, so the outline can never disagree with the silhouette), and every
// piece that could carry a diagonal edge through a pixel center is emitted
// twice with different triangulation. AA stays off: fringes blend toward the
// black clear color and read as dark halos.
constexpr float kPi = 3.14159265358979323846f;
constexpr int   kCornerSeg = 12;                  // arc segments per 90 degrees
constexpr float kCornerQuarter = kPi * 0.5f;
// Angle of corner arc point i. Fill pies and outline annuli both call this,
// so their vertices are float-identical.
inline float CornerAngle(float a0, int i) {
    return a0 + ((float)i / (float)kCornerSeg) * kCornerQuarter;
}
inline void PanelCorners(ImVec2 a, ImVec2 b, float r, ImVec2 centers[4], float starts[4]) {
    centers[0] = ImVec2(b.x - r, a.y + r); starts[0] = -kCornerQuarter; // top-right
    centers[1] = ImVec2(b.x - r, b.y - r); starts[1] = 0.0f;            // bottom-right
    centers[2] = ImVec2(a.x + r, b.y - r); starts[2] = kCornerQuarter;  // bottom-left
    centers[3] = ImVec2(a.x + r, a.y + r); starts[3] = kPi;             // top-left
}
// Solid rect: full quad plus top/bottom halves. The halves' fan diagonals
// differ from the full quad's, so a pixel center sitting exactly on one
// diagonal is strictly inside another piece instead of being dropped.
inline void SolidRect(ImDrawList* dl, ImVec2 pmin, ImVec2 pmax, ImU32 col) {
    if (pmax.x <= pmin.x || pmax.y <= pmin.y) return;
    const ImVec2 q[4] = { pmin, ImVec2(pmax.x, pmin.y), pmax, ImVec2(pmin.x, pmax.y) };
    dl->AddConvexPolyFilled(q, 4, col);
    const float ym = (pmin.y + pmax.y) * 0.5f;
    const ImVec2 t[4] = { pmin, ImVec2(pmax.x, pmin.y), ImVec2(pmax.x, ym), ImVec2(pmin.x, ym) };
    dl->AddConvexPolyFilled(t, 4, col);
    const ImVec2 b[4] = { ImVec2(pmin.x, ym), ImVec2(pmax.x, ym), pmax, ImVec2(pmin.x, pmax.y) };
    dl->AddConvexPolyFilled(b, 4, col);
}
// Quarter disc (center + inscribed arc polygon), emitted with two fan
// anchors (center and arc point v3). A fan drops pixel centers that lie
// exactly on its edges; the two anchors' edge sets only meet at the arc
// vertices, so chord and radial lines are always covered by the other fan.
inline void SolidPie(ImDrawList* dl, ImVec2 c, float r, float a0, ImU32 col) {
    ImVec2 pts[kCornerSeg + 2];
    pts[0] = c;
    for (int i = 0; i <= kCornerSeg; ++i) {
        const float a = CornerAngle(a0, i);
        pts[1 + i] = ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r);
    }
    dl->AddConvexPolyFilled(pts, kCornerSeg + 2, col);
    const int v3 = 1 + kCornerSeg / 4;   // re-anchor the fan at arc point v3
    ImVec2 pts2[kCornerSeg + 2];
    for (int i = 0; i <= kCornerSeg + 1; ++i)
        pts2[i] = pts[(v3 + i <= kCornerSeg + 1) ? (v3 + i) : (v3 + i - (kCornerSeg + 2))];
    dl->AddConvexPolyFilled(pts2, kCornerSeg + 2, col);
}
inline void PanelFill(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float r) {
    // No AA here: this silhouette touches transparency and the compositor
    // keys out only pure black, so AA fringe (blended toward the black clear
    // color) renders as a background-dependent dark halo that reads as
    // dirty/transparent corners on bright scenes.
    ImDrawListFlags saved = dl->Flags;
    dl->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);
    ImVec2 centers[4]; float starts[4];
    PanelCorners(a, b, r, centers, starts);
    for (int c = 0; c < 4; ++c)
        SolidPie(dl, centers[c], r, starts[c], col);
    // Fill rects; the center one reaches 1px into the corner squares so the
    // shared tangent vertices can never drop a pixel (still inside the
    // silhouette: the reach is at most sqrt(2) < r from an arc center).
    const float ex = (r * 0.5f < 1.0f) ? r * 0.5f : 1.0f;
    SolidRect(dl, ImVec2(a.x + r - ex, a.y + r - ex), ImVec2(b.x - r + ex, b.y - r + ex), col);
    SolidRect(dl, ImVec2(a.x + r, a.y), ImVec2(b.x - r, a.y + r), col);
    SolidRect(dl, ImVec2(a.x + r, b.y - r), ImVec2(b.x - r, b.y), col);
    SolidRect(dl, ImVec2(a.x, a.y + r), ImVec2(a.x + r, b.y - r), col);
    SolidRect(dl, ImVec2(b.x - r, a.y + r), ImVec2(b.x, b.y - r), col);
    dl->Flags = saved;
}
// Panel fill: uniform flat fill inside the rounded silhouette. Everything
// stays INSIDE: our compositor is colourkey-based (only pure black is
// see-through), so anything low-alpha painted outside the fill would blend
// against black and land as an opaque near-black fringe. AA is disabled in
// PanelFill/FrostedPanelStroke for the same reason.
inline void FrostedPanelFill(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 fill, float r) {
    // Uniform fill only: no offset shadow (asymmetric dark fringe on
    // bottom/right) and no white diffusion sheen (which lightened the panel
    // header/footer vs the opaque rows and made side-by-side panels differ).
    PanelFill(dl, a, b, fill, r);
}

// The 1px UIStroke: a uniform 1px frame — 1px deep on the straight edges and
// 1px radially on the corner arcs (a heavier 2px corner band reads wrong
// against the 1px straights). Fully opaque with AA off, so the color
// is perfectly even: no fringe blending against the varying row colors
// underneath (dark rows vs bright enabled rows).
//
// Built as filled geometry sharing the fill's exact corner arc vertices, so
// the outline hugs the silhouette at every sub-pixel panel position (the old
// PathStroke version offset each segment with approximate rsqrt and emitted
// unjoined per-segment quads: 1px edges dropped/doubled on integer offsets,
// wedge gaps at corner vertices, and a corner radius that read larger than
// the fill's where the band met the straight stroke).
inline void FrostedPanelStroke(ImDrawList* dl, ImVec2 a, ImVec2 b, float r) {
    const ImU32 col = IM_COL32(60, 60, 60, 255);
    ImDrawListFlags saved = dl->Flags;
    dl->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);
    // 1px straight edges, flush with the silhouette (depth d in [0,1])
    SolidRect(dl, ImVec2(a.x + r, a.y), ImVec2(b.x - r, a.y + 1.0f), col);
    SolidRect(dl, ImVec2(b.x - 1.0f, a.y + r), ImVec2(b.x, b.y - r), col);
    SolidRect(dl, ImVec2(a.x + r, b.y - 1.0f), ImVec2(b.x - r, b.y), col);
    SolidRect(dl, ImVec2(a.x, a.y + r), ImVec2(a.x + 1.0f, b.y - r), col);
    // 1px corner bands: quarter annuli (r-1..r) — same depth as the straights
    // for a uniform frame weight. Outer boundary = the fill's corner polygon
    // itself. Each quad is nudged a fraction of a degree into its neighbours
    // so adjacent quads never share a diagonal edge line (a shared diagonal
    // through a pixel center would be rejected by both and punch a gap).
    const float ri = r - 1.0f;
    const float dq = 0.15f * kPi / 180.0f;
    ImVec2 centers[4]; float starts[4];
    PanelCorners(a, b, r, centers, starts);
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i < kCornerSeg; ++i) {
            const float a0 = CornerAngle(starts[c], i) - dq;
            const float a1 = CornerAngle(starts[c], i + 1) + dq;
            const float c0 = cosf(a0), s0 = sinf(a0);
            const float c1 = cosf(a1), s1 = sinf(a1);
            const ImVec2 q[4] = {
                ImVec2(centers[c].x + c0 * ri, centers[c].y + s0 * ri),
                ImVec2(centers[c].x + c0 * r,  centers[c].y + s0 * r),
                ImVec2(centers[c].x + c1 * r,  centers[c].y + s1 * r),
                ImVec2(centers[c].x + c1 * ri, centers[c].y + s1 * ri),
            };
            dl->AddConvexPolyFilled(q, 4, col);
        }
    }
    dl->Flags = saved;
}

inline void FrostedPanel(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 fill, float r) {
    FrostedPanelFill(dl, a, b, fill, r);
    FrostedPanelStroke(dl, a, b, r);
}

inline bool ContainsCI(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    std::string h = hay, n = needle;
    for (auto& c : h) c = (char)tolower((unsigned char)c);
    for (auto& c : n) c = (char)tolower((unsigned char)c);
    return h.find(n) != std::string::npos;
}

inline ImVec4 AccentForIndex(const vape::Accent& a, int index, float t) {
    (void)index; // one shared hue: per-row offsets made rainbow desync across the UI
    float h = a.hue, s = a.sat, v = a.val;
    if (a.rainbow) {
        h = fmodf(t * 0.1f * a.rainbowSpeed, 1.0f);
        if (h < 0) h += 1.0f;
        vape::VapeRainbow(h, s, v);
    }
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
    // Opaque, with the near-black floor from ColorkeySafe: the old
    // min(s,v)/0.05 alpha fade made the whole accent (and the HSV sliders'
    // knob + fill) vanish as saturation approached 0, so saturation 0 plus
    // the value slider couldn't drive a black-white accent.
    return vape::ColorkeySafe(ImVec4(r, g, b, 1.0f));
}

inline void AccentHSV(const vape::Accent& a, int index, float t, float& h, float& s, float& v) {
    (void)index; // shared hue, see AccentForIndex
    if (a.rainbow) {
        h = fmodf(t * 0.1f * a.rainbowSpeed, 1.0f);
        if (h < 0) h += 1.0f;
        vape::VapeRainbow(h, s, v);
    } else {
        h = a.hue; s = a.sat; v = a.val;
    }
}


// All rows share one width (uniform, full-bleed). The old enabled/full vs
// disabled/1px-short split left a 1px panel-colored seam on disabled rows
// and made the (translucent) outline blend differently per row.
inline float RowWidth(bool fullBleed) {
    (void)fullBleed;
    float w = ImGui::GetContentRegionAvail().x;
    if (w >= 10.0f) {
        return w;
    }
    ImGui::SetCursorPosX(0.0f); // callers are always inside a content child
    return ImGui::GetWindowWidth();
}

// Clip a label to maxW pixels with an ellipsis, so fixed-column text can never
// run past the divider / panel border it is aligned to.
inline std::string TruncateToWidth(const std::string& s, float maxW) {
    if (maxW <= 4.0f || ImGui::CalcTextSize(s.c_str()).x <= maxW)
        return s;
    std::string out = s;
    while (!out.empty() && ImGui::CalcTextSize((out + "...").c_str()).x > maxW)
        out.pop_back();
    return out.empty() ? std::string() : out + "...";
}

