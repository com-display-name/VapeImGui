// Legit Modules window (LegitWindow.lua): standalone grid window that swaps
// in for the whole main GUI, with the sliding options pane.
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

// ============================= Legit Modules GUI (LegitWindow.lua) =============================
// A standalone 700x380 window that replaces the whole main GUI while open.
// Header: legit icon + close (min.png); a 242x29 "Search mods" box top-right;
// the module list is a 4-wide grid of 163x114 cards (toggle pill, dots, name
// at the bottom); dots/right-click slides a 220px options pane over the
// window behind a 50% shadow, reusing the category option widgets.
void VapeGui::RenderLegit(vape::Category& cat) {
    float t = (float)ImGui::GetTime();
    using namespace vape;
    ImVec4 bg = MainColor();
    ImVec4 txt = TextColor();
    ImGuiIO& io = ImGui::GetIO();
    if (legitX < 0.0f || legitY < 0.0f) { // centred on open, like the reference
        legitX = io.DisplaySize.x * 0.5f - kLegitW * 0.5f;
        legitY = io.DisplaySize.y * 0.5f - kLegitH * 0.5f;
    }

    ImGui::SetNextWindowPos(ImVec2(legitX, legitY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kLegitW, kLegitH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeLegit", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 mp = io.MousePos;
    bool paneOpenedNow = false; // the click that opens the pane must not close it again
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + kLegitW, wp.y + kLegitH), ToU32(bg), kRadius);

        // header drag (right 60px reserved for the close button). Skipped while the
        // options pane is open: its movement is suppressed anyway, and ImGui gives
        // hover to the first item in the rect -- this button would steal the pane's
        // back arrow (491,13) which sits inside the drag area.
        if (!legitSettings) {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##legit_drag", ImVec2(kLegitW - 60, 40));
            if (ImGui::IsItemActive()) {
                legitX += io.MouseDelta.x;
                legitY += io.MouseDelta.y;
            }
        }
        DrawIcon("legit_mode_icon.png", ImVec2(wp.x + 18, wp.y + 11), ImVec2(16, 16), ToU32(txt));

        // close (min.png): first click eats the options pane, second returns
        // to the main GUI (the reference eats the pane via its shadow).
        ImGui::SetCursorPos(ImVec2(kLegitW - 31, 11));
        ImGui::InvisibleButton("##legit_close", ImVec2(16, 16));
        bool clHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            if (legitSettings) legitSettings = nullptr;
            else if (vape::Category* lc = FindCategory("Legit")) lc->open = false;
        }
        DrawIcon("min.png", ImVec2(wp.x + kLegitW - 31, wp.y + 11), ImVec2(16, 16),
                 ToU32(Lighten(bg, clHover ? 0.37f : 0.24f)));

        // search box (242x29 at (W-253, 42), stroke lightens on hover)
        ImVec2 hb0 = ImVec2(wp.x + kLegitW - 253, wp.y + 42);
        ImVec2 hb1 = ImVec2(hb0.x + 242, hb0.y + 29);
        bool hbHover = mp.x >= hb0.x && mp.x < hb1.x && mp.y >= hb0.y && mp.y < hb1.y;
        dl->AddRectFilled(hb0, hb1, ToU32(Darken(bg, 0.02f)), 4.0f);
        dl->AddRect(hb0, hb1, ToU32(Lighten(bg, hbHover ? 0.0875f : 0.02f)), 4.0f);
        DrawIcon("search.png", ImVec2(hb1.x - 25, hb0.y + 9), ImVec2(12, 12),
                 ToU32(Lighten(bg, 0.42f)));
        if (!legitSettings) { // shadow blocks input while the pane is open
            ImGui::SetCursorScreenPos(ImVec2(hb0.x + 8, hb0.y));
            ImGui::PushItemWidth(205);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ToU32(Darken(txt, 0.16f)));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 7)); // 15 + 14 = 29
            ImGui::InputTextWithHint("##legit_search", "Search mods", legitBuf, IM_ARRAYSIZE(legitBuf));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopItemWidth();
        }

        // card grid: 4 columns of 163x114 cards, 6px gaps, in a 684x301 area
        // that scrolls with the wheel (LegitWindow.lua UIGridLayout).
        std::vector<vape::Module*> vis;
        for (auto& m : cat.modules)
            if (legitBuf[0] == '\0' || ContainsCI(m.name, legitBuf))
                vis.push_back(&m);
        ImGui::SetCursorPos(ImVec2(14, 76));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::BeginChild("##legit_grid", ImVec2(684, 301), false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoScrollbar);
        // Card art stays on the parent drawlist (so the options pane shadow draws
        // over it), but must be clipped to the grid rect: without this a
        // wheel-scrolled grid paints cards over the header/search box.
        ImVec2 g0 = ImGui::GetWindowPos(); // child outer rect
        ImVec2 g1 = ImGui::GetWindowSize();
        dl->PushClipRect(g0, ImVec2(g0.x + g1.x, g0.y + g1.y), true);
        for (int i = 0; i < (int)vis.size(); ++i) {
            vape::Module& m = *vis[i];
            ImGui::SetCursorPos(ImVec2((i % 4) * 169.0f, (i / 4) * 120.0f));
            std::string id = "##lcard_" + m.name;
            ImGui::InvisibleButton(id.c_str(), ImVec2(163, 114));
            bool hov = ImGui::IsItemHovered();
            ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
            (void)r1;
            // dots hit area (14x24 at (136,9)) beats the card's own LMB: one click
            // either opens the config or toggles the module, never both.
            ImVec2 d0 = ImVec2(r0.x + 136, r0.y + 9), d1 = ImVec2(d0.x + 14, d0.y + 24);
            bool inDots = hov && mp.x >= d0.x && mp.x < d1.x && mp.y >= d0.y && mp.y < d1.y;
            if (!legitSettings) {
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !inDots) {
                    m.enabled = !m.enabled;
                    if (toggleNotifications)
                        Notify(m.name.c_str(), m.enabled ? "Enabled" : "Disabled", 1.5f,
                               m.enabled ? 0 : 1);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    legitSettings = &m;
                if (inDots && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    legitSettings = &m;
                    paneOpenedNow = true;
                }
            }
            dl->AddRectFilled(r0, ImVec2(r0.x + 163, r0.y + 114),
                              ToU32(m.enabled || hov ? Lighten(bg, 0.05f) : Lighten(bg, 0.02f)), 4.0f);

            // dots glyph (2x12 at (142,15))
            DrawIcon("overlaydots.png", ImVec2(r0.x + 142, r0.y + 15), ImVec2(2, 12),
                     ToU32(inDots ? txt : Lighten(bg, 0.37f)));

            // toggle pill (22x12 at (106,15), 8x8 knob like Toggle.lua)
            ImVec4 modAccent = AccentForIndex(accent, m.index, t);
            ImVec2 h0 = ImVec2(r0.x + 106, r0.y + 15), h1 = ImVec2(h0.x + 22, h0.y + 12);
            dl->AddRectFilled(h0, h1, ToU32(m.enabled ? modAccent : Lighten(bg, 0.14f)), 6.0f);
            float kx = m.enabled ? h1.x - 10.0f : h0.x + 2.0f;
            dl->AddRectFilled(ImVec2(kx, h0.y + 2), ImVec2(kx + 8, h0.y + 10), ToU32(bg), 4.0f);

            // module name (13px at (16,81))
            ImVec4 nameCol = m.enabled ? Lighten(txt, 0.2f) : Darken(txt, 0.31f);
            dl->AddText(ImGui::GetFont(), 13.0f, ImVec2(r0.x + 16, r0.y + 81),
                        ToU32(nameCol), m.name.c_str());
            if (hov && !legitSettings && !m.tooltip.empty())
                ShowTooltip(m.tooltip.c_str());
        }
        dl->PopClipRect();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        // options pane: 50% shadow over the window + a 220px pane on the right
        if (legitSettings) {
            vape::Module* lm = legitSettings; // stable: back-click only clears the ptr
            dl->AddRectFilled(wp, ImVec2(wp.x + kLegitW, wp.y + kLegitH), IM_COL32(0, 0, 0, 128));
            ImVec2 p0 = ImVec2(wp.x + kLegitW - 220, wp.y);
            ImVec2 p1 = ImVec2(wp.x + kLegitW, wp.y + kLegitH - 5);
            dl->AddRectFilled(p0, p1, ToU32(bg), 4.0f);
            // back (back.png 16x16 at (11,13))
            ImGui::SetCursorScreenPos(ImVec2(p0.x + 11, p0.y + 13));
            ImGui::InvisibleButton("##legit_back", ImVec2(16, 16));
            bool bkHover = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked())
                legitSettings = nullptr;
            DrawIcon("back.png", ImVec2(p0.x + 11, p0.y + 13), ImVec2(16, 16),
                     ToU32(bkHover ? txt : Lighten(bg, 0.37f)));
            // title (module name, 13px at (36,12))
            dl->AddText(ImGui::GetFont(), 13.0f, ImVec2(p0.x + 36, p0.y + 12),
                        ToU32(Darken(txt, 0.16f)), lm->name.c_str());
            // options list at y=41 (module options reuse the category widgets)
            ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + 41));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::BeginChild("##legit_opts", ImVec2(220, kLegitH - 49), false,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar);
            ImVec4 optAccent = AccentForIndex(accent, lm->index, t);
            float unused = 0.0f;
            for (auto& o : lm->options) {
                switch (o.type) {
                case vape::OptionType::Toggle: RenderToggleOption(*lm, o.toggle, optAccent, 0, unused); break;
                case vape::OptionType::Slider: RenderSliderOption(o.slider, optAccent, 0, unused); break;
                case vape::OptionType::TwoSlider: RenderTwoSliderOption(o.twoslider, optAccent, 0, unused); break;
                case vape::OptionType::Dropdown: RenderDropdownOption(o.dropdown, 0, unused); break;
                case vape::OptionType::TextBox: RenderTextBoxOption(o.textbox, 0, unused); break;
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            // clicking the shadow (outside the pane) closes the pane -- but never
            // the same click that opened it (paneOpenedNow)
            if (legitSettings && !paneOpenedNow && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !(mp.x >= p0.x && mp.x < p1.x && mp.y >= p0.y && mp.y < p1.y))
                legitSettings = nullptr;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{legitX, legitY, legitX + kLegitW, legitY + kLegitH});
}
