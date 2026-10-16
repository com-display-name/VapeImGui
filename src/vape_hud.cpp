// Foreground HUD overlays: the text-GUI arraylist and the notification stack.
// Both draw on ImGui's foreground drawlist so they float above every panel.
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

// ============================= Text GUI (arraylist) =============================
void VapeGui::RenderTextGui(const ImVec4& accentRgb) {
    if (!textGuiEnabled)
        return;
    struct Entry { std::string name; ImVec4 col; };
    std::vector<Entry> entries;
    float t = (float)ImGui::GetTime();
    for (auto& c : categories)
        for (auto& m : c.modules)
            if (m.enabled && m.visible)
                entries.push_back({m.name, AccentForIndex(accent, m.index, t)});
    if (entries.empty())
        return;
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.name < b.name; });

    ImDrawList* fg = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = ImGui::GetFont();
    float fs = ImGui::GetFontSize();
    float x = io.DisplaySize.x - 10.0f;
    float y = 10.0f;
    for (auto& e : entries) {
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, e.name.c_str());
        float w = ts.x + 26.0f;
        float h = 22.0f;
        PanelFill(fg, ImVec2(x - w, y), ImVec2(x, y + h), IM_COL32(10, 10, 10, 190), 4.0f);
        fg->AddRectFilled(ImVec2(x - w, y + 2), ImVec2(x - w + 3, y + h - 2), ToU32(e.col), 1.0f);
        fg->AddText(ImVec2(x - w + 9, y + (h - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), e.name.c_str());
        y += h + 3.0f;
    }
    (void)accentRgb;
}

// ============================= Notifications =============================
void VapeGui::RenderNotifications() {
    float t = VapeTime();
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = ImGui::GetFont();
    float fs = ImGui::GetFontSize();

    // prune expired
    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
                       [t](const vape::Notification& n) { return t - n.created > n.duration + 0.4f; }),
        notifications.end());

    float y = io.DisplaySize.y - 20.0f;
    for (int i = (int)notifications.size() - 1; i >= 0; --i) {
        const auto& n = notifications[i];
        float age = t - n.created;
        float slide = std::min(age / 0.4f, 1.0f);
        slide = 1.0f - (1.0f - slide) * (1.0f - slide); // ease out
        // slide-out over the last 0.35s, accelerating away to the right.
        // Pure translation - the compositor keys on black, so an alpha fade
        // would land as a near-black panel.
        float outK = age > n.duration ? std::min((age - n.duration) / 0.35f, 1.0f) : 0.0f;
        outK = outK * outK * (3.0f - 2.0f * outK); // smoothstep

        ImVec2 titleTs = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, n.title.c_str());
        ImVec2 textTs = font->CalcTextSizeA(fs * 0.92f, FLT_MAX, 0.0f, n.text.c_str());
        float w = std::max({titleTs.x, textTs.x}) + 52.0f; // text at +37, 15px right pad
        w = std::max(w, 170.0f);                           // hugs content like the reference
        float h = 60.0f;
        float x1 = io.DisplaySize.x - 10.0f - w * slide + (w + 20.0f) * outK;
        float y1 = y - h;
        if (y1 < 10) break;

        FrostedPanel(fg, ImVec2(x1, y1), ImVec2(x1 + w, y1 + h), IM_COL32(18, 18, 18, 245), 8.0f);

        // small icon aligned with the title row; badge rect measured from the PNGs
        // (info: 120x120 canvas, solid 24x24 badge at (48,48); alert: 126x116,
        // 33x30 at (46,43)). UV-cropped to the badge itself: both PNGs carry a
        // wide low-alpha (1-9) white glow around it, and the canvas pokes outside
        // the panel — off the panel that glow lands on transparent black, and the
        // colorkey compositor (pure black only) turned it into an opaque
        // near-black disc around the icon.
        ImVec2 g0 = ImVec2(x1 + 9, y1 + 11);
        const char* iconName = n.kind == 2 ? "noti_alert.png" : "noti_info.png";
        auto it = icons.find(iconName);
        if (it != icons.end() && it->second.srv) {
            float s = n.kind == 2 ? 18.0f / 30.0f : 18.0f / 24.0f;
            float bx = n.kind == 2 ? 46.0f : 48.0f;
            float by = n.kind == 2 ? 43.0f : 48.0f;
            float bw = n.kind == 2 ? 33.0f : 24.0f;
            float bh = n.kind == 2 ? 30.0f : 24.0f;
            float cw = n.kind == 2 ? 126.0f : 120.0f;
            float ch = n.kind == 2 ? 116.0f : 120.0f;
            // geometry rect = the badge itself; the UV crop selects the same
            // region, so the badge lands at 18px instead of stretching over
            // the whole canvas rect.
            ImVec2 i1 = ImVec2(g0.x + bw * s, g0.y + bh * s);
            // clip to the panel: even the badge's antialiased edge is low-alpha,
            // and the canvas extends past the panel silhouette, so without the
            // clip that edge would also blend against transparent black.
            fg->PushClipRect(ImVec2(x1, y1), ImVec2(x1 + w, y1 + h), true);
            fg->AddImage((ImTextureID)it->second.srv, g0, i1,
                         ImVec2(bx / cw, by / ch), ImVec2((bx + bw) / cw, (by + bh) / ch),
                         IM_COL32(255, 255, 255, 255));
            fg->PopClipRect();
        } else {
            fg->AddCircleFilled(ImVec2(g0.x + 9, g0.y + 9), 9.0f, IM_COL32(255, 255, 255, 255));
            fg->AddText(ImVec2(g0.x + 6.5f, g0.y + 2), IM_COL32(0, 0, 0, 255), "i");
        }
        ImU32 titleCol = n.kind == 2 ? IM_COL32(250, 50, 56, 255) : IM_COL32(255, 255, 255, 255);
        fg->AddText(ImVec2(x1 + 37, y1 + 11), titleCol, n.title.c_str());

        // subtitle carries the state colour: red = disabled, green = enabled
        ImU32 subCol = IM_COL32(170, 170, 170, 255);
        if (n.kind == 2 || n.kind == 1)
            subCol = IM_COL32(250, 50, 56, 255);
        else if (n.text == "Enabled")
            subCol = IM_COL32(90, 210, 130, 255);
        fg->AddText(font, fs * 0.92f, ImVec2(x1 + 37, y1 + 32), subCol, n.text.c_str());

        // timeout line, flush with the bottom edge (white for info, red for errors)
        float frac = std::clamp(1.0f - age / n.duration, 0.0f, 1.0f);
        ImU32 barCol = n.kind == 2 ? IM_COL32(250, 50, 56, 255) : IM_COL32(255, 255, 255, 255);
        fg->AddRectFilled(ImVec2(x1 + 8, y1 + h - 3), ImVec2(x1 + 8 + (w - 16) * frac, y1 + h), barCol);
        y = y1 - 8.0f;
    }
}

