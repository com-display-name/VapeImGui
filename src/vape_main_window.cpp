// Main clickgui window (GUI.lua): panel size, category lookup, header,
// category rows and footer buttons.
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

// ============================= Main window =============================
ImVec2 VapeGui::MainSize() const {
    using namespace vape;
    // GUI.lua: window.Size = UDim2.fromOffset(220, 42 + content) - the row list
    // starts at y=37 and keeps a 5px tail so the square rows clear the rounded
    // bottom corners (37 + rows*40 + 5). +1 row for the Friends list button.
    // The legit menu is not a tab, so it doesn't count towards the rows.
    int rows = 1;
    for (auto& c : categories)
        if (!c.isLegit)
            rows++;
    float winH = kRowsTop + (float)rows * kModuleH + kBotGap;
    return ImVec2(kPanelW, winH);
}

vape::Category* VapeGui::FindCategory(const std::string& name) {
    for (auto& c : categories)
        if (c.name == name)
            return &c;
    return nullptr;
}

void VapeGui::RenderMain() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    using namespace vape;
    // All panels share MainColor so the main window and category windows are
    // the identical shade side-by-side (no Dark-vs-Main two-tone).
    ImVec4 bg = MainColor();
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);

    float winH = MainSize().y;

    ImGui::SetNextWindowPos(ImVec2(mainX, mainY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, winH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeMain", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), ToU32(bg), kRadius);

        // Header drag zone (rows start at y=37, so the grip stops there too)
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##main_drag", ImVec2(kPanelW - 80, kRowsTop));
        if (ImGui::IsItemActive()) {
            mainX += ImGui::GetIO().MouseDelta.x;
            mainY += ImGui::GetIO().MouseDelta.y;
        }
        // Logo: vape text + v4 badge tinted accent (GUI.lua: logo 12,11 / v4 +54)
        DrawIcon("vapelogomini.png", ImVec2(wp.x + 12, wp.y + 11), ImVec2(55, 16), IM_COL32(255, 255, 255, 255));
        DrawIcon("v4mini.png", ImVec2(wp.x + 66, wp.y + 11), ImVec2(23, 16), ToU32(accentRgb));

        // Discord button (GUI.lua: 16x16 image at (W-56, 11))
        ImGui::SetCursorPos(ImVec2(kPanelW - 56, 11));
        ImGui::InvisibleButton("##discord", ImVec2(16, 16));
        bool discordHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            ImGui::SetClipboardText("https://discord.gg/VZEQJxMSnG");
            Notify("Discord", "Invite copied to clipboard", 2.0f);
        }
        DrawIcon("discord.png", ImVec2(wp.x + kPanelW - 56, wp.y + 11),
                 ImVec2(16, 16), IM_COL32(255, 255, 255, 255)); // always the bright (hover) tint
        if (discordHover) ShowTooltip("Join discord");

        // Settings gear (GUI.lua: 40x40 at (W-40, 0), icon 14x14 at +15,+12)
        ImGui::SetCursorPos(ImVec2(kPanelW - 40, 0));
        ImGui::InvisibleButton("##gear", ImVec2(40, kRowsTop));
        bool gearHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            settingsOpen = !settingsOpen;
            if (settingsOpen) settingsPage = 0;
        }
        DrawIcon("settings.png", ImVec2(wp.x + kPanelW - 25, wp.y + 12), ImVec2(14, 14),
                 gearHover ? ToU32(txt) : ToU32(Lighten(MainColor(), 0.37f)));
        if (gearHover) ShowTooltip("Open settings");

        // Search lives in its own floating bar at the top of the screen
        // (SearchBar.lua), so the main window is header + category rows only.

        // Category rows (GUIButton.lua: full-width 220x40 buttons, icon 16,13)
        float ry = kRowsTop;
        for (auto& c : categories) {
            if (c.isLegit) continue; // legit menu lives behind the search-bar switch
            // Full-bleed row: it runs to the panel edge (inset 0). The UIStroke
            // is redrawn over it afterwards so the border stays on top.
            ImGui::SetCursorPos(ImVec2(0, ry));
            std::string id = "##catbtn_" + c.name;
            ImGui::InvisibleButton(id.c_str(), ImVec2(kPanelW, kModuleH));
            bool hov = ImGui::IsItemHovered();
            bool clk = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            if (clk) {
                c.open = !c.open;
                if (c.open) Notify(c.name, "Category opened", 1.2f);
            }
            ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
            // Uniform rows: same Main bg idle whether the category is open or
            // closed (open is shown by accent text/icon only). Hover alone
            // lightens for feedback so idle categories never differ in shade.
            ImVec4 rowBg = hov ? Lighten(MainColor(), 0.02f) : MainColor();
            dl->AddRectFilled(r0, r1, ToU32(rowBg));
            ImVec4 nameCol = c.open ? accentRgb : (hov ? txt : dim);
            DrawIcon(c.iconFile.c_str(), ImVec2(r0.x + 16, r0.y + 13), ImVec2(14, 14), ToU32(nameCol));
            dl->AddText(ImVec2(r0.x + 37, r0.y + 12), ToU32(nameCol), c.name.c_str());
            // right chevron (reference: 4x8 at W-20,16)
            DrawIcon("expandarrow.png", ImVec2(r1.x - 20, r0.y + 16), ImVec2(4, 8),
                     ToU32(c.open ? accentRgb : Lighten(MainColor(), 0.37f)));
            ry += kModuleH;
        }
        // Friends list button (CategoryList.lua registers a main-window
        // GUIButton that toggles its window).
        {
            ImGui::SetCursorPos(ImVec2(0, ry));
            ImGui::InvisibleButton("##catbtn_Friends", ImVec2(kPanelW, kModuleH));
            bool hov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                friendsOpen = !friendsOpen;
                if (friendsOpen) Notify("Friends", "Category opened", 1.2f);
            }
            ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
            dl->AddRectFilled(r0, r1, ToU32(hov ? Lighten(MainColor(), 0.02f) : MainColor()));
            ImVec4 nameCol = friendsOpen ? accentRgb : (hov ? txt : dim);
            DrawIcon("friends.png", ImVec2(r0.x + 16, r0.y + 13), ImVec2(14, 14), ToU32(nameCol));
            dl->AddText(ImVec2(r0.x + 37, r0.y + 12), ToU32(nameCol), "Friends");
            DrawIcon("expandarrow.png", ImVec2(r1.x - 20, r0.y + 16), ImVec2(4, 8),
                     ToU32(friendsOpen ? accentRgb : Lighten(MainColor(), 0.37f)));
        }
        // Redraw the UIStroke on top of the rows (Roblox draws a frame's stroke
        // over its children) so full-bleed rows never swallow the border.
        FrostedPanelStroke(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), kRadius);
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{mainX, mainY, mainX + kPanelW, mainY + winH});
}
