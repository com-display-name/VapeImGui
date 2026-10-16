// Settings panes: shared row widgets (toggles, nav/action buttons, HSV
// sliders, swatches, display selector) plus the Settings/Visual/Debug pages.
#include "vape_gui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "vape_internal.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

// ============================= Settings row widgets =============================
// Shared by the Settings / Visual / Debug panes.
bool VapeGui::VapeToggleRow(const char* label, bool& v, const char* id, const ImVec4& accentRgb) {
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    // settings rows are outside any module: always full-bleed (current size)
    float w = RowWidth(true);
    ImGui::InvisibleButton(id, ImVec2(w, 30));
    bool changed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    if (changed)
        v = !v;
    bool hov = ImGui::IsItemHovered();
    ImVec2 a0 = ImGui::GetItemRectMin(), a1 = ImGui::GetItemRectMax();
    ImDrawList* d2 = ImGui::GetWindowDrawList();
    // Same Main base as every other settings row/panel so idle rows match.
    d2->AddRectFilled(a0, a1, ToU32(MainColor()));
    d2->AddText(ImVec2(a0.x + 10, a0.y + 7), ToU32(hov || v ? txt : dim), label);
    ImVec2 h0 = ImVec2(a1.x - 30, a0.y + 9);
    ImVec2 h1 = ImVec2(a1.x - 8, a0.y + 21);
    ImVec4 hc = v ? accentRgb : (hov ? Lighten(MainColor(), 0.37f) : Lighten(MainColor(), 0.14f));
    d2->AddRectFilled(h0, h1, ToU32(hc), 6.0f);
    float kx = v ? (h1.x - 10) : (h0.x + 2);
    d2->AddRectFilled(ImVec2(kx, h0.y + 2), ImVec2(kx + 8, h0.y + 10), ToU32(MainColor()), 4.0f);
    return changed;
}

// Branch-style navigation button: opens a settings sub-page (Visual/Debug).
// Uniform Main bg idle, lightened on hover only; returns true when clicked.
bool VapeGui::VapeNavButton(const char* label, const char* id, const ImVec4& accentRgb) {
    (void)accentRgb;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    // settings row: outside any module, always full-bleed (current size)
    float w = RowWidth(true);
    ImGui::InvisibleButton(id, ImVec2(w, 40));
    bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool hov = ImGui::IsItemHovered();
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(r0, r1, ToU32(hov ? Lighten(MainColor(), 0.02f) : MainColor()));
    dl->AddText(ImVec2(r0.x + 14, r0.y + 12), ToU32(hov ? txt : dim), label);
    DrawIcon("expandarrow.png", ImVec2(r1.x - 18, r0.y + 16), ImVec2(5, 9),
             ToU32(Lighten(MainColor(), 0.37f)));
    return clicked;
}

bool VapeGui::VapeActionButton(const char* label, const char* id) {
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    // settings row: outside any module, always full-bleed (current size)
    float w = RowWidth(true);
    ImGui::InvisibleButton(id, ImVec2(w, 34));
    bool hov = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    ImVec2 u0 = ImGui::GetItemRectMin(), u1 = ImGui::GetItemRectMax();
    ImDrawList* d2 = ImGui::GetWindowDrawList();
    // Same Main base idle so Reset/Uninject match the rows above them.
    d2->AddRectFilled(u0, u1, ToU32(hov ? Lighten(MainColor(), 0.02f) : MainColor()));
    ImVec2 lsz = ImGui::CalcTextSize(label);
    d2->AddText(ImVec2(u0.x + (w - lsz.x) * 0.5f, u0.y + 9), ToU32(hov ? txt : dim), label);
    return clicked;
}

void VapeGui::VapeHSVSliders() {
    using namespace vape;
    ImVec4 dim = Darken(TextColor(), 0.16f);
    ImVec4 accentRgb = AccentForIndex(accent, 0, (float)ImGui::GetTime());
    struct HSVSlider { const char* name; float* v; };
    HSVSlider sliders[] = {{"Hue", &accent.hue}, {"Saturation", &accent.sat}, {"Value", &accent.val}};
    for (auto& s : sliders) {
        // settings pane row: outside any module, always full-bleed (current size)
        float w = RowWidth(true);
        std::string sid = std::string("##hsv_") + s.name;
        ImGui::InvisibleButton(sid.c_str(), ImVec2(w, 50));
        ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
        bool active = ImGui::IsItemActive();
        ImDrawList* d2 = ImGui::GetWindowDrawList();
        d2->AddRectFilled(r0, r1, ToU32(MainColor()));
        d2->AddText(ImVec2(r0.x + 10, r0.y + 4), ToU32(dim), s.name);
        char vb[16];
        snprintf(vb, sizeof(vb), "%d", (int)(*s.v * 100));
        ImVec2 vsz = ImGui::CalcTextSize(vb);
        d2->AddText(ImVec2(r1.x - 10 - vsz.x, r0.y + 4), ToU32(dim), vb);
        ImVec2 t0 = ImVec2(r0.x + 10, r0.y + 37);
        ImVec2 t1 = ImVec2(r1.x - 10, r0.y + 39);
        // rainbow gradient for Hue
        if (std::string(s.name) == "Hue") {
            const int segs = 24;
            for (int i = 0; i < segs; ++i) {
                float f0 = (float)i / segs, f1 = (float)(i + 1) / segs;
                d2->AddRectFilled(ImVec2(t0.x + (t1.x - t0.x) * f0, t0.y),
                                  ImVec2(t0.x + (t1.x - t0.x) * f1, t1.y),
                                  ToU32(HSVtoRGB(f0, 1, 1)));
            }
        } else {
            d2->AddRectFilled(t0, t1, ToU32(Lighten(MainColor(), 0.034f)));
        }
        float frac = std::clamp(*s.v, 0.04f, 0.96f);
        float fx = t0.x + (t1.x - t0.x) * frac;
        if (std::string(s.name) != "Hue")
            d2->AddRectFilled(t0, ImVec2(fx, t1.y), ToU32(accentRgb));
        else
            d2->AddRectFilled(ImVec2(fx - 1, t0.y - 3), ImVec2(fx + 1, t1.y + 3), IM_COL32(255, 255, 255, 255));
        d2->AddCircleFilled(ImVec2(fx, (t0.y + t1.y) * 0.5f), 7.0f, ToU32(accentRgb));
        d2->AddCircle(ImVec2(fx, (t0.y + t1.y) * 0.5f), 7.0f, IM_COL32(255, 255, 255, 90));
        ImVec2 mp = ImGui::GetIO().MousePos;
        if (active && mp.y > r0.y + 20) {
            *s.v = std::clamp((mp.x - t0.x) / (t1.x - t0.x), 0.0f, 1.0f);
        }
    }
}

void VapeGui::VapePresetSwatches() {
    using namespace vape;
    ImVec4 dim = Darken(TextColor(), 0.16f);
    ImU32 presets[] = {
        IM_COL32(250, 50, 56, 255), IM_COL32(242, 99, 33, 255), IM_COL32(252, 179, 22, 255),
        IM_COL32(5, 133, 104, 255), IM_COL32(47, 122, 229, 255), IM_COL32(126, 84, 217, 255),
        IM_COL32(232, 96, 152, 255),
    };
    // settings pane row: outside any module, always full-bleed (current size)
    float w = RowWidth(true);
    ImGui::InvisibleButton("##swatches", ImVec2(w, 40));
    ImVec2 q0 = ImGui::GetItemRectMin();
    ImDrawList* d2 = ImGui::GetWindowDrawList();
    d2->AddRectFilled(q0, ImVec2(q0.x + w, q0.y + 40), ToU32(MainColor()));
    d2->AddText(ImVec2(q0.x + 10, q0.y + 2), ToU32(dim), "Presets");
    float sx = q0.x + 10;
    for (int i = 0; i < 7; ++i) {
        ImVec2 c0 = ImVec2(sx, q0.y + 18);
        ImVec2 c1 = ImVec2(sx + 20, q0.y + 34);
        d2->AddRectFilled(c0, c1, presets[i], 3.0f);
        ImVec2 mp = ImGui::GetIO().MousePos;
        bool inside = mp.x >= c0.x && mp.x <= c1.x && mp.y >= c0.y && mp.y <= c1.y;
        if (inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            float r = ((presets[i] >> 0) & 255) / 255.0f;
            float g = ((presets[i] >> 8) & 255) / 255.0f;
            float b = ((presets[i] >> 16) & 255) / 255.0f;
            float h, s, v;
            ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);
            accent.hue = h; accent.sat = s; accent.val = v;
        }
        sx += 26;
    }
}

void VapeGui::VapeDisplaySelector() {
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    // settings row: outside any module, always full-bleed (current size)
    float w = RowWidth(true);
    ImGui::InvisibleButton("##display_dd", ImVec2(w, 34));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        monitorDropdownOpen = !monitorDropdownOpen;
    bool hov = ImGui::IsItemHovered();
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    ImDrawList* d2 = ImGui::GetWindowDrawList();
    d2->AddRectFilled(r0, r1, ToU32(MainColor()));
    d2->AddText(ImVec2(r0.x + 10, r0.y + 9), ToU32(hov ? txt : dim), "Display");
    std::string cur = (monitorIndex >= 0 && monitorIndex < (int)monitorNames.size())
                          ? monitorNames[monitorIndex]
                          : "Default";
    float maxW = w - 110.0f;
    std::string shown = cur;
    while (shown.size() > 4 && ImGui::CalcTextSize(shown.c_str()).x > maxW)
        shown.pop_back();
    if (shown.size() != cur.size())
        shown += "...";
    ImVec2 ssz = ImGui::CalcTextSize(shown.c_str());
    d2->AddText(ImVec2(r1.x - 26 - ssz.x, r0.y + 9), ToU32(dim), shown.c_str());
    ImVec2 ap = ImVec2(r1.x - 18, r0.y + 15);
    ImU32 acol = ToU32(Lighten(MainColor(), 0.37f));
    if (monitorDropdownOpen)
        d2->AddTriangleFilled(ImVec2(ap.x, ap.y), ImVec2(ap.x + 9, ap.y),
                              ImVec2(ap.x + 4.5f, ap.y + 4.5f), acol);
    else
        d2->AddTriangleFilled(ImVec2(ap.x, ap.y + 4.5f), ImVec2(ap.x + 9, ap.y + 4.5f),
                              ImVec2(ap.x + 4.5f, ap.y), acol);
    if (monitorDropdownOpen) {
        for (int i = 0; i < (int)monitorNames.size(); ++i) {
            std::string iid = "##moni_" + std::to_string(i);
            ImGui::InvisibleButton(iid.c_str(), ImVec2(w, 26));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (i != monitorIndex) {
                    monitorIndex = i;
                    if (onMonitorChange)
                        onMonitorChange(i);
                }
                monitorDropdownOpen = false;
            }
            bool ih = ImGui::IsItemHovered();
            ImVec2 i0 = ImGui::GetItemRectMin(), i1 = ImGui::GetItemRectMax();
            if (ih)
                d2->AddRectFilled(i0, i1, ToU32(Lighten(MainColor(), 0.06f)));
            d2->AddText(ImVec2(i0.x + 22, i0.y + 5),
                        ToU32(i == monitorIndex ? txt : dim), monitorNames[i].c_str());
        }
    }
}

// ============================= Visual pane =============================
void VapeGui::RenderVisual() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    (void)t;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    float pw = vape::kPanelW;
    float ph = VisualSize().y;

    ImGui::SetNextWindowPos(ImVec2(visualX, visualY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(pw, ph), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeVisual", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + pw, wp.y + ph), ToU32(MainColor()), kRadius);

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##visual_drag", ImVec2(pw - 40, 41));
        if (ImGui::IsItemActive()) {
            visualX += ImGui::GetIO().MouseDelta.x;
            visualY += ImGui::GetIO().MouseDelta.y;
        }
        dl->AddText(ImVec2(wp.x + 14, wp.y + 12), ToU32(txt), "Visual");

        ImGui::SetCursorPos(ImVec2(pw - 40, 0));
        ImGui::InvisibleButton("##visual_close", ImVec2(40, 34));
        if (ImGui::IsItemClicked())
            visualOpen = false;
        DrawIcon("close.png", ImVec2(wp.x + pw - 26, wp.y + 10), ImVec2(12, 12), ToU32(dim));
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 38), ImVec2(wp.x + pw, wp.y + 39), IM_COL32(255, 255, 255, 18));

        ImGui::SetCursorPos(ImVec2(0, 42));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // flush rows, like vape
        ImGui::BeginChild("##visual_list", ImVec2(pw, ph - 46 - kRadius), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoScrollbar);
        VapeToggleRow("Rainbow", accent.rainbow, "##v_rainbow", accentRgb);
        VapeHSVSliders();
        VapePresetSwatches();
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{visualX, visualY, visualX + pw, visualY + ph});
}

ImVec2 VapeGui::VisualSize() const {
    return ImVec2(vape::kPanelW, 41 + 30 + 3 * 50 + 40 + 8 + vape::kRadius);
}

// ============================= Debug pane =============================
void VapeGui::RenderDebug() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    (void)t;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    float pw = vape::kPanelW;
    float ph = DebugSize().y;

    ImGui::SetNextWindowPos(ImVec2(debugX, debugY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(pw, ph), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeDebug", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + pw, wp.y + ph), ToU32(MainColor()), kRadius);

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##debug_drag", ImVec2(pw - 40, 41));
        if (ImGui::IsItemActive()) {
            debugX += ImGui::GetIO().MouseDelta.x;
            debugY += ImGui::GetIO().MouseDelta.y;
        }
        dl->AddText(ImVec2(wp.x + 14, wp.y + 12), ToU32(txt), "Debug");

        ImGui::SetCursorPos(ImVec2(pw - 40, 0));
        ImGui::InvisibleButton("##debug_close", ImVec2(40, 34));
        if (ImGui::IsItemClicked())
            debugOpen = false;
        DrawIcon("close.png", ImVec2(wp.x + pw - 26, wp.y + 10), ImVec2(12, 12), ToU32(dim));
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 38), ImVec2(wp.x + pw, wp.y + 39), IM_COL32(255, 255, 255, 18));

        ImGui::SetCursorPos(ImVec2(0, 42));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // flush rows, like vape
        ImGui::BeginChild("##debug_list", ImVec2(pw, ph - 46 - kRadius), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoScrollbar);
        VapeDisplaySelector();
        if (VapeToggleRow("Always on top", alwaysOnTop, "##d_topmost", accentRgb)) {
            if (onTopmostChange)
                onTopmostChange(alwaysOnTop);
        }
        if (VapeToggleRow("Fullscreen", fullscreen, "##d_fullscreen", accentRgb)) {
            if (onFullscreenChange)
                onFullscreenChange(fullscreen);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{debugX, debugY, debugX + pw, debugY + ph});
}

// ============================= Settings pane =============================
ImVec2 VapeGui::DebugSize() const {
    float listH = monitorDropdownOpen ? (float)monitorNames.size() * 26.0f : 0.0f;
    return ImVec2(vape::kPanelW, 41 + 34 + listH + 30 + 30 + 30 + 8 + vape::kRadius);
}

ImVec2 VapeGui::SettingsSize() const {
    using namespace vape;
    float listH = (settingsPage == 2 && monitorDropdownOpen) ? (float)monitorNames.size() * 26.0f : 0.0f;
    // Branch navigation: the pane swaps its whole content to the sub-page.
    // Page 0 (root): 3 toggles + Visual + Debug nav buttons + 2 actions.
    // Page 1 (Visual): rainbow + hue/sat/val + presets.
    // Page 2 (Debug): display (+ dropdown) + 3 toggles.
    float rows = 0.0f;
    if (settingsPage == 1) {
        rows = 30 + 3 * 50 + 40; // rainbow + hue/sat/val + presets
    } else if (settingsPage == 2) {
        rows = 34 + listH + 30 + 30 + 30; // display + 3 toggles
    } else {
        rows = 30 + 30 + 30 + 40 + 40 + 34 + 34;
    }
    // The pane sits on top of the main window and only grows past it.
    float h = std::max(41.0f + rows + kBotGap, MainSize().y);
    return ImVec2(kPanelW, h);
}

void VapeGui::RenderSettings() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    (void)t;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);

    // Reference (SettingsPane.lua): the pane covers the main window, so it sits
    // exactly on it and only grows downwards when its rows are taller.
    float px = mainX;
    float py = mainY;
    float pw = kPanelW;
    float ph = SettingsSize().y;

    ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(pw, ph), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeSettings", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + pw, wp.y + ph), ToU32(MainColor()), kRadius);
        const char* pageTitle = settingsPage == 1 ? "Visual" : (settingsPage == 2 ? "Debug" : "Settings");
        // back arrow: on root it closes the pane, on a sub-page it returns
        // to the root (branch navigation, like vape SettingsPanes).
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##settings_back", ImVec2(30, kRowsTop));
        if (ImGui::IsItemClicked()) {
            if (settingsPage != 0)
                settingsPage = 0;
            else {
                settingsOpen = false;
                settingsPage = 0;
            }
        }
        bool backHover = ImGui::IsItemHovered();
        DrawIcon("backmini.png", ImVec2(wp.x + 12, wp.y + 14), ImVec2(14, 14),
                 ToU32(backHover ? txt : Lighten(MainColor(), 0.37f)));
        dl->AddText(ImVec2(wp.x + 36, wp.y + 12), ToU32(txt), pageTitle);
        // mini close (reference: 20x20 button at (W-28, 11))
        ImGui::SetCursorPos(ImVec2(pw - 28, 11));
        ImGui::InvisibleButton("##settings_close", ImVec2(20, 20));
        bool closeHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) { settingsOpen = false; settingsPage = 0; }
        DrawIcon("close.png", ImVec2(wp.x + pw - 24, wp.y + 15), ImVec2(12, 12),
                 ToU32(closeHover ? txt : dim));
        // divider at y=41 - the row list starts right there
        dl->AddRectFilled(ImVec2(wp.x, wp.y + 41), ImVec2(wp.x + pw, wp.y + 42), IM_COL32(255, 255, 255, 18));

        // full-bleed content column (0px inset): rows run to the panel edge and
        // the UIStroke is redrawn over them afterwards
        ImGui::SetCursorPos(ImVec2(0, 41));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // flush rows, like vape
        ImGui::BeginChild("##settings_list", ImVec2(pw, ph - 49), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoScrollbar);
        ImDrawList* sdl = ImGui::GetWindowDrawList();

        if (settingsPage == 1) {
            // Visual sub-page (entire pane content swaps here).
            VapeToggleRow("Rainbow", accent.rainbow, "##s_rainbow", accentRgb);
            VapeHSVSliders();
            VapePresetSwatches();
        } else if (settingsPage == 2) {
            // Debug sub-page (entire pane content swaps here).
            VapeDisplaySelector();
            if (VapeToggleRow("Always on top", alwaysOnTop, "##d_topmost", accentRgb)) {
                if (onTopmostChange)
                    onTopmostChange(alwaysOnTop);
            }
            if (VapeToggleRow("Fullscreen", fullscreen, "##d_fullscreen", accentRgb)) {
                if (onFullscreenChange)
                    onFullscreenChange(fullscreen);
            }
        } else {
            VapeToggleRow("Text GUI", textGuiEnabled, "##s_textgui", accentRgb);
            VapeToggleRow("Toggle notifications", toggleNotifications, "##s_notif", accentRgb);
            // screenproof: hidden from screen capture/sharing (SetWindowDisplayAffinity)
            if (VapeToggleRow("Screenproof", screenproof, "##s_screenproof", accentRgb)) {
                if (onScreenproofChange)
                    onScreenproofChange(screenproof);
            }
            if (VapeNavButton("Visual", "##s_visual", accentRgb))
                settingsPage = 1;
            if (VapeNavButton("Debug", "##s_debug", accentRgb))
                settingsPage = 2;

        // Reset positions
        if (VapeActionButton("Reset positions", "##resetpos")) {
            ResetPositions();
            Notify("Vape", "Panel positions reset", 2.0f);
        }

        // Uninject: clear every module and stop the process. PostQuitMessage
        // makes the host's message pump see WM_QUIT, which runs the normal
        // shutdown (ImGui + D3D + window) - same path as pressing END.
        if (VapeActionButton("Uninject", "##uninject")) {
            for (auto& c : categories)
                for (auto& m : c.modules)
                    m.enabled = false;
            PostQuitMessage(0);
        }
        } // end root page (settingsPage == 0)

        // UIStroke drawn over the rows: the child's clip only covers the list
        // region, so widen it back to the panel rect for the stroke pass.
        sdl->PushClipRect(wp, ImVec2(wp.x + pw, wp.y + ph), false);
        FrostedPanelStroke(sdl, wp, ImVec2(wp.x + pw, wp.y + ph), kRadius);
        sdl->PopClipRect();
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{px, py, px + pw, py + ph});
}
