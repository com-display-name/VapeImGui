#include "../third_party/imgui/imgui_internal.h"
#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::RenderSearch() {
    using namespace Vape;
    ImVec4 Background = MainColor();
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImVec4 AccentColor = GetAccentColor(Accent, (float)ImGui::GetTime());

    struct SearchResult {
        Vape::Category* Category;
        Vape::Module* Module;
    };
    std::vector<SearchResult> Results;
    if (SearchBuffer[0] != '\0') {
        for (auto& Category : Categories) {
            for (auto& Module : Category.Modules) {
                if (!ContainsIgnoreCase(Module.Name, SearchBuffer))
                    continue;
                Results.push_back(SearchResult{&Category, &Module});
                if (Results.size() >= 10)
                    break;
            }
            if (Results.size() >= 10)
                break;
        }
    }
    float ListHeight = (float)Results.size() * ModuleHeight;
    float WindowHeight = SearchHeight + (ListHeight > 0.0f ? ListHeight + BottomGap : 0.0f);

    ImGuiIO& Io = ImGui::GetIO();
    if (SearchX < 0.0f) {
        SearchX = Io.DisplaySize.x * 0.5f - PanelWidth * 0.5f;
        SearchY = SearchTop;
    }
    ImVec2 Position = ImVec2(SearchX, SearchY);

    bool Open = BeginPanel("##VapeSearch", Position, ImVec2(PanelWidth, WindowHeight));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), ToU32(Background),
                  PanelRadius);
        DrawIcon("search.png", ImVec2(WindowPosition.x + PanelWidth - 25, WindowPosition.y + 11),
                 ImVec2(14, 14), ToU32(Lighten(MainColor(), 0.37f)));

        Vape::Category* LegitCategory = FindCategory("Legit");
        ImGui::SetCursorPos(ImVec2(8, 11));
        ImGui::InvisibleButton("##legit_switch", ImVec2(29, 16));
        bool LegitSwitchHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && LegitCategory) {
            LegitCategory->Open = true;
            LegitX = LegitY = -1.0f;
            LegitSettings = nullptr;
        }
        DrawIcon("legit_switch.png", ImVec2(WindowPosition.x + 8, WindowPosition.y + 11), ImVec2(29, 16),
                 ToU32(AccentColor));
        if (LegitSwitchHovered)
            ShowTooltip("Legit modules");

        DrawList->AddRectFilled(ImVec2(WindowPosition.x + 43, WindowPosition.y + 13),
                                ImVec2(WindowPosition.x + 45, WindowPosition.y + 25),
                                ToU32(Lighten(Background, 0.14f)));

        ImGui::SetCursorPos(ImVec2(50, 0));
        ImGui::PushItemWidth(PanelWidth - 50);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ToU32(Text));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 11));
        ImGui::InputTextWithHint("##search", "Search...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopItemWidth();

        if (DraggingSearch) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                DraggingSearch = false;
            else {
                SearchX += Io.MouseDelta.x;
                SearchY += Io.MouseDelta.y;
            }
        } else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 6.0f)) {
            ImGui::ClearActiveID();
            DraggingSearch = true;
            SearchX += Io.MouseDelta.x;
            SearchY += Io.MouseDelta.y;
        }

        if (!Results.empty()) {
            ImGui::SetCursorPos(ImVec2(0, SearchHeight));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            BeginScrollableChild("##search_results", ImVec2(PanelWidth, ListHeight));
            for (auto& Result : Results) {
                std::string WidgetId = "##search_" + Result.Category->Name + "_" + Result.Module->Name;
                ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(PanelWidth, ModuleHeight));
                bool Hovered = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    Result.Module->Enabled = !Result.Module->Enabled;
                    NotifyModuleState(*Result.Module);
                }
                ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
                ImVec4 RowBackground = Result.Module->Enabled
                                           ? GetAccentColor(Accent, (float)ImGui::GetTime())
                                           : (Hovered ? Lighten(MainColor(), 0.02f) : MainColor());
                DrawList->AddRectFilled(RowMin, RowMax, ToU32(RowBackground));

                float AccentHue, AccentSaturation, AccentValue;
                GetAccentHsv(Accent, (float)ImGui::GetTime(), AccentHue, AccentSaturation, AccentValue);
                ImVec4 NameColor = Result.Module->Enabled
                                       ? Vape::GetAccentTextColor(AccentHue, AccentSaturation, AccentValue)
                                       : (Hovered ? Text : DimText);

                const float CategoryX = 12.0f, DividerX = 92.0f, NameX = 103.0f;
                std::string CategoryName =
                    TruncateToWidth(Result.Category->Name, DividerX - CategoryX - 6.0f);
                std::string ModuleName =
                    TruncateToWidth(Result.Module->Name, (RowMax.x - RowMin.x) - NameX - 8.0f);
                DrawList->AddText(ImVec2(RowMin.x + CategoryX, RowMin.y + 12), ToU32(DimText),
                                  CategoryName.c_str());
                DrawList->AddRectFilled(ImVec2(RowMin.x + DividerX, RowMin.y + 11),
                                        ImVec2(RowMin.x + DividerX + 1.0f, RowMin.y + 29),
                                        IM_COL32(255, 255, 255, 45));
                DrawList->AddText(ImVec2(RowMin.x + NameX, RowMin.y + 12), ToU32(NameColor),
                                  ModuleName.c_str());
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();

            DrawPanelBorder(DrawList, WindowPosition,
                            ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight),
                            PanelRadius);
        }
    }
    EndPanel();

    HitRects.push_back(HitRect{Position.x, Position.y, Position.x + PanelWidth, Position.y + WindowHeight});
}
