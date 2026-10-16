// Floating search bar (SearchBar.lua): top-centre filter box with the
// live results list underneath.
#include "vape_gui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "vape_internal.h"
#include "../third_party/imgui/imgui_internal.h" // ImGui::ClearActiveID (bar drag)

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

// ============================= Floating search bar =============================
// SearchBar.lua: a detached 220x37 bar pinned to the top centre of the screen
// that grows downwards with its results list (37 + rows, capped at 10 rows the
// way the reference caps its 400px result canvas).
void VapeGui::RenderSearch() {
    using namespace vape;
    ImVec4 bg = MainColor();
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImVec4 accentRgb = AccentForIndex(accent, 0, (float)ImGui::GetTime());

    struct Hit { vape::Category* c; vape::Module* m; };
    std::vector<Hit> hits;
    if (searchBuf[0] != '\0') {
        for (auto& c : categories) {
            for (auto& m : c.modules) {
                if (!ContainsCI(m.name, searchBuf)) continue;
                hits.push_back(Hit{&c, &m});
                if (hits.size() >= 10) break;
            }
            if (hits.size() >= 10) break;
        }
    }
    float listH = (float)hits.size() * kModuleH;
    float winH = kSearchH + (listH > 0.0f ? listH + kBotGap : 0.0f);

    ImGuiIO& io = ImGui::GetIO();
    if (searchX < 0.0f) { // not placed yet (startup / Reset positions): top centre
        searchX = io.DisplaySize.x * 0.5f - kPanelW * 0.5f;
        searchY = kSearchTopY;
    }
    ImVec2 pos = ImVec2(searchX, searchY);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, winH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeSearch", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), ToU32(bg), kRadius);
        DrawIcon("search.png", ImVec2(wp.x + kPanelW - 25, wp.y + 11), ImVec2(14, 14),
                 ToU32(Lighten(MainColor(), 0.37f)));

        // Legit switch (SearchBar.lua: legit_switch.png 29x16 at (8,11),
        // always V4-logo/accent tinted). Clicking swaps to the Legit Modules
        // GUI - the whole main GUI hides until the window's close button.
        vape::Category* legitCat = FindCategory("Legit");
        ImGui::SetCursorPos(ImVec2(8, 11));
        ImGui::InvisibleButton("##legit_switch", ImVec2(29, 16));
        bool lsHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && legitCat) {
            legitCat->open = true;
            legitX = legitY = -1.0f; // re-centre on open, like the reference
            legitSettings = nullptr;
        }
        DrawIcon("legit_switch.png", ImVec2(wp.x + 8, wp.y + 11), ImVec2(29, 16), ToU32(accentRgb));
        if (lsHover) ShowTooltip("Legit modules");
        // divider (SearchBar.lua: 2x12 at (43,13))
        dl->AddRectFilled(ImVec2(wp.x + 43, wp.y + 13), ImVec2(wp.x + 45, wp.y + 25),
                          ToU32(Lighten(bg, 0.14f)));

        // Transparent input to the right of the divider (SearchBar.lua starts
        // the box at offset 50; kSearchH is the reference bar height).
        ImGui::SetCursorPos(ImVec2(50, 0));
        ImGui::PushItemWidth(kPanelW - 50);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ToU32(txt));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 11)); // 15 + 22 = 37
        ImGui::InputTextWithHint("##search", "Search...", searchBuf, IM_ARRAYSIZE(searchBuf));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopItemWidth();

        // Drag: a plain click focuses the field; holding and moving grabs the
        // bar itself. The first moving frame drops the caret (otherwise the
        // InputText would start selecting text) and applies the delta right
        // away - that keeps the cursor inside the bar's hit rect, or the
        // overlay stops receiving mouse moves and the drag would freeze.
        if (searchDrag) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                searchDrag = false;
            else {
                searchX += io.MouseDelta.x;
                searchY += io.MouseDelta.y;
            }
        } else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 6.0f)) {
            ImGui::ClearActiveID();
            searchDrag = true;
            searchX += io.MouseDelta.x;
            searchY += io.MouseDelta.y;
        }

        if (!hits.empty()) {
            ImGui::SetCursorPos(ImVec2(0, kSearchH));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // flush rows, like vape
            ImGui::BeginChild("##search_results", ImVec2(kPanelW, listH), false,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar);
            for (auto& h : hits) {
                std::string id = "##search_" + h.c->name + "_" + h.m->name;
                ImGui::InvisibleButton(id.c_str(), ImVec2(kPanelW, kModuleH));
                bool hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    h.m->enabled = !h.m->enabled;
                    Notify(h.m->name.c_str(), h.m->enabled ? "Enabled" : "Disabled", 1.5f,
                           h.m->enabled ? 0 : 1);
                }
                ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
                ImVec4 rowBg = h.m->enabled
                                   ? AccentForIndex(accent, h.m->index, (float)ImGui::GetTime())
                                   : (hov ? Lighten(MainColor(), 0.02f) : MainColor());
                dl->AddRectFilled(r0, r1, ToU32(rowBg));
                // text contrast from the LIVE accent: rainbow cycles the hue,
                // so the static base hue picked the wrong text color mid-cycle
                float ah, as, av;
                AccentHSV(accent, h.m->index, (float)ImGui::GetTime(), ah, as, av);
                ImVec4 nameCol = h.m->enabled ? vape::AccentText(ah, as, av)
                                              : (hov ? txt : dim);
                // "Tab | ElementName" - the bar sits in a fixed column so every
                // result lines up, whatever the tab name happens to be.
                const float tabX = 12.0f, divX = 92.0f, nameX = 103.0f;
                std::string tab = TruncateToWidth(h.c->name, divX - tabX - 6.0f);
                std::string elem = TruncateToWidth(h.m->name, (r1.x - r0.x) - nameX - 8.0f);
                dl->AddText(ImVec2(r0.x + tabX, r0.y + 12), ToU32(dim), tab.c_str());
                dl->AddRectFilled(ImVec2(r0.x + divX, r0.y + 11), ImVec2(r0.x + divX + 1.0f, r0.y + 29),
                                  IM_COL32(255, 255, 255, 45));
                dl->AddText(ImVec2(r0.x + nameX, r0.y + 12), ToU32(nameCol), elem.c_str());
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            // UIStroke redrawn over the full-bleed rows.
            FrostedPanelStroke(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), kRadius);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{pos.x, pos.y, pos.x + kPanelW, pos.y + winH});
}
