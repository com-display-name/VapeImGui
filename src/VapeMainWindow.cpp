#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

ImVec2 VapeGui::GetMainSize() const {
    using namespace Vape;

    int Rows = 1;
    for (auto& Category : Categories)
        if (!Category.IsLegit)
            Rows++;
    float WindowHeight = RowsTop + (float)Rows * ModuleHeight + BottomGap;
    return ImVec2(PanelWidth, WindowHeight);
}

Vape::Category* VapeGui::FindCategory(const std::string& Name) {
    for (auto& Category : Categories)
        if (Category.Name == Name)
            return &Category;
    return nullptr;
}

void VapeGui::RenderMain() {
    float Time = (float)ImGui::GetTime();
    ImVec4 AccentColor = GetAccentColor(Accent, Time);
    using namespace Vape;

    ImVec4 Background = MainColor();
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float WindowHeight = GetMainSize().y;

    bool Open = BeginPanel("##VapeMain", ImVec2(MainX, MainY), ImVec2(PanelWidth, WindowHeight));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), ToU32(Background),
                  PanelRadius);

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##main_drag", ImVec2(PanelWidth - 80, RowsTop));
        if (ImGui::IsItemActive()) {
            MainX += ImGui::GetIO().MouseDelta.x;
            MainY += ImGui::GetIO().MouseDelta.y;
        }

        DrawIcon("vapelogomini.png", ImVec2(WindowPosition.x + 12, WindowPosition.y + 11), ImVec2(55, 16),
                 IM_COL32(255, 255, 255, 255));
        DrawIcon("v4mini.png", ImVec2(WindowPosition.x + 66, WindowPosition.y + 11), ImVec2(23, 16),
                 ToU32(AccentColor));

        ImGui::SetCursorPos(ImVec2(PanelWidth - 56, 11));
        ImGui::InvisibleButton("##discord", ImVec2(16, 16));
        bool DiscordHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            ImGui::SetClipboardText("https://discord.gg/VZEQJxMSnG");
            Notify("Discord", "Invite copied to clipboard", 2.0f);
        }
        DrawIcon("discord.png", ImVec2(WindowPosition.x + PanelWidth - 56, WindowPosition.y + 11),
                 ImVec2(16, 16), IM_COL32(255, 255, 255, 255));
        if (DiscordHover)
            ShowTooltip("Join discord");

        ImGui::SetCursorPos(ImVec2(PanelWidth - 40, 0));
        ImGui::InvisibleButton("##gear", ImVec2(40, RowsTop));
        bool GearHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            SettingsOpen = !SettingsOpen;
            if (SettingsOpen)
                SettingsPage = Vape::SettingsView::Root;
        }
        DrawIcon("settings.png", ImVec2(WindowPosition.x + PanelWidth - 25, WindowPosition.y + 12),
                 ImVec2(14, 14), GearHover ? ToU32(Text) : ToU32(Lighten(MainColor(), 0.37f)));
        if (GearHover)
            ShowTooltip("Open settings");

        float RowY = RowsTop;
        for (auto& Category : Categories) {
            if (Category.IsLegit)
                continue;

            ImGui::SetCursorPos(ImVec2(0, RowY));
            std::string WidgetId = "##catbtn_" + Category.Name;
            ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(PanelWidth, ModuleHeight));
            bool Hovered = ImGui::IsItemHovered();
            bool Clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            if (Clicked) {
                Category.Open = !Category.Open;
                if (Category.Open)
                    Notify(Category.Name, "Category opened", 1.2f);
            }
            ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();

            ImVec4 RowBackground = Hovered ? Lighten(MainColor(), 0.02f) : MainColor();
            DrawList->AddRectFilled(RowMin, RowMax, ToU32(RowBackground));
            ImVec4 NameColor = Category.Open ? AccentColor : (Hovered ? Text : DimText);
            DrawIcon(Category.IconFile.c_str(), ImVec2(RowMin.x + 16, RowMin.y + 13), ImVec2(14, 14),
                     ToU32(NameColor));
            DrawList->AddText(ImVec2(RowMin.x + 37, RowMin.y + 12), ToU32(NameColor), Category.Name.c_str());

            DrawIcon("expandarrow.png", ImVec2(RowMax.x - 20, RowMin.y + 16), ImVec2(4, 8),
                     ToU32(Category.Open ? AccentColor : Lighten(MainColor(), 0.37f)));
            RowY += ModuleHeight;
        }

        {
            ImGui::SetCursorPos(ImVec2(0, RowY));
            ImGui::InvisibleButton("##catbtn_Friends", ImVec2(PanelWidth, ModuleHeight));
            bool Hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                FriendsOpen = !FriendsOpen;
                if (FriendsOpen)
                    Notify("Friends", "Category opened", 1.2f);
            }
            ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
            DrawList->AddRectFilled(RowMin, RowMax,
                                    ToU32(Hovered ? Lighten(MainColor(), 0.02f) : MainColor()));
            ImVec4 NameColor = FriendsOpen ? AccentColor : (Hovered ? Text : DimText);
            DrawIcon("friends.png", ImVec2(RowMin.x + 16, RowMin.y + 13), ImVec2(14, 14), ToU32(NameColor));
            DrawList->AddText(ImVec2(RowMin.x + 37, RowMin.y + 12), ToU32(NameColor), "Friends");
            DrawIcon("expandarrow.png", ImVec2(RowMax.x - 20, RowMin.y + 16), ImVec2(4, 8),
                     ToU32(FriendsOpen ? AccentColor : Lighten(MainColor(), 0.37f)));
        }

        DrawPanelBorder(DrawList, WindowPosition,
                        ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), PanelRadius);
    }
    EndPanel();

    HitRects.push_back(HitRect{MainX, MainY, MainX + PanelWidth, MainY + WindowHeight});
}
