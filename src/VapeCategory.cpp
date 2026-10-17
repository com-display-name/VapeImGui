#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

ImVec2 VapeGui::GetCategorySize(const Vape::Category& Category) const {
    using namespace Vape;

    float ContentHeight = 0;
    for (auto& Module : Category.Modules)
        if (Module.Visible || EditMode)
            ContentHeight += GetModuleHeight(Module);

    float WindowHeight =
        HeaderHeight + (Category.Expanded ? std::min(ContentHeight, MaxContentHeight) + BottomGap : 0.0f);
    return ImVec2(PanelWidth, std::max(WindowHeight, HeaderHeight));
}

void VapeGui::RenderCategory(Vape::Category& Category) {
    float Time = (float)ImGui::GetTime();
    using namespace Vape;
    ImVec4 Background = MainColor();
    ImVec4 Text = TextColor();
    ImVec4 Gray = ImVec4(140.0f / 255, 140.0f / 255, 140.0f / 255, 1);

    float WindowHeight = GetCategorySize(Category).y;

    std::string WindowId = "##Cat_" + Category.Name;
    bool Open = BeginPanel(WindowId.c_str(), ImVec2(Category.PositionX, Category.PositionY),
                           ImVec2(PanelWidth, std::max(WindowHeight, HeaderHeight)));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), ToU32(Background),
                  PanelRadius);

        ImGui::SetCursorPos(ImVec2(0, 0));
        std::string DragId = "##catdrag_" + Category.Name;
        ImGui::InvisibleButton(DragId.c_str(), ImVec2(PanelWidth - 62, HeaderHeight));
        if (ImGui::IsItemActive()) {
            Category.PositionX += ImGui::GetIO().MouseDelta.x;
            Category.PositionY += ImGui::GetIO().MouseDelta.y;
        }
        bool HeaderHover = ImGui::IsItemHovered();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            Category.Expanded = !Category.Expanded;

        DrawIcon(Category.IconFile.c_str(), ImVec2(WindowPosition.x + 12, WindowPosition.y + 13),
                 ImVec2(15, 15), ToU32(Text));
        DrawList->AddText(ImVec2(WindowPosition.x + 33, WindowPosition.y + 12), ToU32(Text),
                          Category.Name.c_str());

        if (!EditMode) {
            if (HeaderHover || ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                ImGui::SetCursorPos(ImVec2(PanelWidth - 49, 0));
                std::string PencilId = "##pencil_" + Category.Name;
                ImGui::InvisibleButton(PencilId.c_str(), ImVec2(20, 40));
                bool PencilHovered = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked())
                    EditMode = true;
                DrawIcon("editlarge.png", ImVec2(WindowPosition.x + PanelWidth - 45, WindowPosition.y + 14),
                         ImVec2(12, 12),
                         ToU32(PencilHovered ? ImVec4(220.0f / 255, 220.0f / 255, 220.0f / 255, 1) : Gray));
            }

            ImGui::SetCursorPos(ImVec2(PanelWidth - 29, 0));
            std::string ArrowId = "##arrow_" + Category.Name;
            ImGui::InvisibleButton(ArrowId.c_str(), ImVec2(27, 40));
            bool ArrowHovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                Category.Expanded = !Category.Expanded;
            ImVec2 ArrowPosition = ImVec2(WindowPosition.x + PanelWidth - 20, WindowPosition.y + 18);

            ImU32 ArrowColor =
                ToU32(ArrowHovered ? ImVec4(220.0f / 255, 220.0f / 255, 220.0f / 255, 1) : Gray);
            auto ArrowIcon = Icons.find("downexpand.png");
            if (ArrowIcon != Icons.end() && ArrowIcon->second.ShaderResource) {
                ImVec2 IconMax = ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4);
                if (Category.Expanded)
                    DrawList->AddImage((ImTextureID)ArrowIcon->second.ShaderResource, ArrowPosition, IconMax,
                                       ImVec2(0, 0), ImVec2(1, 1), ArrowColor);
                else
                    DrawList->AddImage((ImTextureID)ArrowIcon->second.ShaderResource, ArrowPosition, IconMax,
                                       ImVec2(0, 1), ImVec2(1, 0), ArrowColor);
            } else if (Category.Expanded)
                DrawList->AddTriangleFilled(
                    ImVec2(ArrowPosition.x, ArrowPosition.y), ImVec2(ArrowPosition.x + 9, ArrowPosition.y),
                    ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y + 4.5f), ArrowColor);
            else
                DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y + 4.5f),
                                            ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4.5f),
                                            ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y), ArrowColor);
        } else {
            ImGui::SetCursorPos(ImVec2(PanelWidth - 73, 0));
            ImGui::InvisibleButton(("##done_" + Category.Name).c_str(), ImVec2(42, 40));
            bool DoneHovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked())
                EditMode = false;
            DrawList->AddText(ImVec2(WindowPosition.x + PanelWidth - 68, WindowPosition.y + 12),
                              ToU32(DoneHovered ? ImVec4(220.0f / 255, 220.0f / 255, 220.0f / 255, 1) : Gray),
                              "DONE");
        }

        if (Category.Expanded) {
            DrawList->AddRectFilled(ImVec2(WindowPosition.x, WindowPosition.y + RowsTop),
                                    ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + RowsTop + 1),
                                    IM_COL32(255, 255, 255, 18));

            ImGui::SetCursorPos(ImVec2(0, RowsTop));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            std::string ChildId = "##list_" + Category.Name;
            BeginScrollableChild(ChildId.c_str(), ImVec2(PanelWidth, WindowHeight - RowsTop - BottomGap));
            ImDrawList* ContentDrawList = ImGui::GetWindowDrawList();

            for (auto& Module : Category.Modules) {
                if (!Module.Visible && !EditMode)
                    continue;

                RenderModuleRow(Module, Time);
            }

            ContentDrawList->PushClipRect(
                ImVec2(WindowPosition.x, WindowPosition.y),
                ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), false);
            DrawPanelBorder(ContentDrawList, WindowPosition,
                            ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight),
                            PanelRadius);
            ContentDrawList->PopClipRect();
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
    }
    EndPanel();

    HitRects.push_back(HitRect{Category.PositionX, Category.PositionY, Category.PositionX + PanelWidth,
                               Category.PositionY + std::max(WindowHeight, HeaderHeight)});
}
