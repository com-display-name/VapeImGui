#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::RenderLegit(Vape::Category& Category) {
    float Time = (float)ImGui::GetTime();
    using namespace Vape;
    ImVec4 Background = MainColor();
    ImVec4 Text = TextColor();
    ImGuiIO& Io = ImGui::GetIO();
    if (LegitX < 0.0f || LegitY < 0.0f) {
        LegitX = Io.DisplaySize.x * 0.5f - LegitWidth * 0.5f;
        LegitY = Io.DisplaySize.y * 0.5f - LegitHeight * 0.5f;
    }

    bool Open = BeginPanel("##VapeLegit", ImVec2(LegitX, LegitY), ImVec2(LegitWidth, LegitHeight));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    ImVec2 MousePosition = Io.MousePos;
    bool PaneOpenedNow = false;
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + LegitWidth, WindowPosition.y + LegitHeight), ToU32(Background),
                  PanelRadius);

        // The drag target must not cover the options pane back button.
        if (!LegitSettings) {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##legit_drag", ImVec2(LegitWidth - 60, 40));
            if (ImGui::IsItemActive()) {
                LegitX += Io.MouseDelta.x;
                LegitY += Io.MouseDelta.y;
            }
        }
        DrawIcon("legit_mode_icon.png", ImVec2(WindowPosition.x + 18, WindowPosition.y + 11), ImVec2(16, 16),
                 ToU32(Text));

        ImGui::SetCursorPos(ImVec2(LegitWidth - 31, 11));
        ImGui::InvisibleButton("##legit_close", ImVec2(16, 16));
        bool CloseHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            if (LegitSettings)
                LegitSettings = nullptr;
            else if (Vape::Category* LegitCategory = FindCategory("Legit"))
                LegitCategory->Open = false;
        }
        DrawIcon("min.png", ImVec2(WindowPosition.x + LegitWidth - 31, WindowPosition.y + 11), ImVec2(16, 16),
                 ToU32(Lighten(Background, CloseHovered ? 0.37f : 0.24f)));

        ImVec2 SearchMin = ImVec2(WindowPosition.x + LegitWidth - 253, WindowPosition.y + 42);
        ImVec2 SearchMax = ImVec2(SearchMin.x + 242, SearchMin.y + 29);
        bool SearchHovered = MousePosition.x >= SearchMin.x && MousePosition.x < SearchMax.x &&
                             MousePosition.y >= SearchMin.y && MousePosition.y < SearchMax.y;
        DrawList->AddRectFilled(SearchMin, SearchMax, ToU32(Darken(Background, 0.02f)), 4.0f);
        DrawList->AddRect(SearchMin, SearchMax, ToU32(Lighten(Background, SearchHovered ? 0.0875f : 0.02f)),
                          4.0f);
        DrawIcon("search.png", ImVec2(SearchMax.x - 25, SearchMin.y + 9), ImVec2(12, 12),
                 ToU32(Lighten(Background, 0.42f)));
        if (!LegitSettings) {
            ImGui::SetCursorScreenPos(ImVec2(SearchMin.x + 8, SearchMin.y));
            ImGui::PushItemWidth(205);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ToU32(Darken(Text, 0.16f)));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 7));
            ImGui::InputTextWithHint("##legit_search", "Search mods", LegitSearchBuffer,
                                     IM_ARRAYSIZE(LegitSearchBuffer));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopItemWidth();
        }

        std::vector<Vape::Module*> VisibleModules;
        for (auto& Module : Category.Modules)
            if (LegitSearchBuffer[0] == '\0' || ContainsIgnoreCase(Module.Name, LegitSearchBuffer))
                VisibleModules.push_back(&Module);
        ImGui::SetCursorPos(ImVec2(14, 76));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        BeginScrollableChild("##legit_grid", ImVec2(684, 301));

        ImVec2 GridPosition = ImGui::GetWindowPos();
        ImVec2 GridSize = ImGui::GetWindowSize();
        DrawList->PushClipRect(GridPosition, ImVec2(GridPosition.x + GridSize.x, GridPosition.y + GridSize.y),
                               true);
        for (int I = 0; I < (int)VisibleModules.size(); ++I) {
            Vape::Module& Module = *VisibleModules[I];
            ImGui::SetCursorPos(ImVec2((I % 4) * 169.0f, (I / 4) * 120.0f));
            std::string WidgetId = "##lcard_" + Module.Name;
            ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(163, 114));
            bool Hovered = ImGui::IsItemHovered();
            ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
            (void)RowMax;

            ImVec2 DotsMin = ImVec2(RowMin.x + 136, RowMin.y + 9),
                   DotsMax = ImVec2(DotsMin.x + 14, DotsMin.y + 24);
            bool InDots = Hovered && MousePosition.x >= DotsMin.x && MousePosition.x < DotsMax.x &&
                          MousePosition.y >= DotsMin.y && MousePosition.y < DotsMax.y;
            if (!LegitSettings) {
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !InDots) {
                    Module.Enabled = !Module.Enabled;
                    if (ToggleNotifications)
                        NotifyModuleState(Module);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    LegitSettings = &Module;
                if (InDots && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    LegitSettings = &Module;
                    PaneOpenedNow = true;
                }
            }
            DrawList->AddRectFilled(
                RowMin, ImVec2(RowMin.x + 163, RowMin.y + 114),
                ToU32(Module.Enabled || Hovered ? Lighten(Background, 0.05f) : Lighten(Background, 0.02f)),
                4.0f);

            DrawIcon("overlaydots.png", ImVec2(RowMin.x + 142, RowMin.y + 15), ImVec2(2, 12),
                     ToU32(InDots ? Text : Lighten(Background, 0.37f)));

            ImVec4 ModuleAccent = GetAccentColor(Accent, Time);
            ImVec2 SwitchMin = ImVec2(RowMin.x + 106, RowMin.y + 15),
                   SwitchMax = ImVec2(SwitchMin.x + 22, SwitchMin.y + 12);
            DrawList->AddRectFilled(SwitchMin, SwitchMax,
                                    ToU32(Module.Enabled ? ModuleAccent : Lighten(Background, 0.14f)), 6.0f);
            float KnobX = Module.Enabled ? SwitchMax.x - 10.0f : SwitchMin.x + 2.0f;
            DrawList->AddRectFilled(ImVec2(KnobX, SwitchMin.y + 2), ImVec2(KnobX + 8, SwitchMin.y + 10),
                                    ToU32(Background), 4.0f);

            ImVec4 NameColor = Module.Enabled ? Lighten(Text, 0.2f) : Darken(Text, 0.31f);
            DrawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(RowMin.x + 16, RowMin.y + 81), ToU32(NameColor),
                              Module.Name.c_str());
            if (Hovered && !LegitSettings && !Module.Tooltip.empty())
                ShowTooltip(Module.Tooltip.c_str());
        }
        DrawList->PopClipRect();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        if (LegitSettings) {
            Vape::Module* SelectedModule = LegitSettings;
            DrawList->AddRectFilled(WindowPosition,
                                    ImVec2(WindowPosition.x + LegitWidth, WindowPosition.y + LegitHeight),
                                    IM_COL32(0, 0, 0, 128));
            ImVec2 PaneMin = ImVec2(WindowPosition.x + LegitWidth - 220, WindowPosition.y);
            ImVec2 PaneMax = ImVec2(WindowPosition.x + LegitWidth, WindowPosition.y + LegitHeight - 5);
            DrawList->AddRectFilled(PaneMin, PaneMax, ToU32(Background), 4.0f);

            ImGui::SetCursorScreenPos(ImVec2(PaneMin.x + 11, PaneMin.y + 13));
            ImGui::InvisibleButton("##legit_back", ImVec2(16, 16));
            bool BackHovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked())
                LegitSettings = nullptr;
            DrawIcon("back.png", ImVec2(PaneMin.x + 11, PaneMin.y + 13), ImVec2(16, 16),
                     ToU32(BackHovered ? Text : Lighten(Background, 0.37f)));

            DrawList->AddText(ImGui::GetFont(), 13.0f, ImVec2(PaneMin.x + 36, PaneMin.y + 12),
                              ToU32(Darken(Text, 0.16f)), SelectedModule->Name.c_str());

            ImGui::SetCursorScreenPos(ImVec2(PaneMin.x, PaneMin.y + 41));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            BeginScrollableChild("##legit_opts", ImVec2(220, LegitHeight - 49));
            ImVec4 OptionAccent = GetAccentColor(Accent, Time);

            RenderModuleOptions(*SelectedModule, OptionAccent);
            ImGui::EndChild();
            ImGui::PopStyleVar();

            if (LegitSettings && !PaneOpenedNow && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !(MousePosition.x >= PaneMin.x && MousePosition.x < PaneMax.x &&
                  MousePosition.y >= PaneMin.y && MousePosition.y < PaneMax.y))
                LegitSettings = nullptr;
        }
    }
    EndPanel();

    HitRects.push_back(HitRect{LegitX, LegitY, LegitX + LegitWidth, LegitY + LegitHeight});
}
