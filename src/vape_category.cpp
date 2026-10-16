// Category window (Category.lua): draggable module list for one category.
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

// ============================= Category window =============================
ImVec2 VapeGui::CategorySize(const vape::Category& cat) const {
    using namespace vape;
    // content height (+ footer strip so square rows never touch the rounded corners)
    float contentH = 0;
    for (auto& m : cat.modules)
        if (m.visible || editMode)
            contentH += ModuleHeight(m);
    // Reference: window height = 41 + content with rows starting at y=37.
    // The list region ends a full kBotGap above the panel bottom so the
    // square full-bleed rows never reach the rounded corner zone (the
    // outline is redrawn rounded on top, but row fill poking into the
    // curve reads as square background corners).
    float winH = kHeaderH + (cat.expanded ? std::min(contentH, kMaxOpenH) + kBotGap : 0.0f);
    return ImVec2(kPanelW, std::max(winH, kHeaderH));
}

void VapeGui::RenderCategory(vape::Category& cat) {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    using namespace vape;
    ImVec4 bg = MainColor();
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImVec4 gray = ImVec4(140.0f / 255, 140.0f / 255, 140.0f / 255, 1);

    float winH = CategorySize(cat).y;

    std::string winId = "##Cat_" + cat.name;
    ImGui::SetNextWindowPos(ImVec2(cat.posX, cat.posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, std::max(winH, kHeaderH)), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin(winId.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), ToU32(bg), kRadius);

        // header drag (exclude right 60px reserved for pencil/arrow)
        ImGui::SetCursorPos(ImVec2(0, 0));
        std::string dragId = "##catdrag_" + cat.name;
        ImGui::InvisibleButton(dragId.c_str(), ImVec2(kPanelW - 62, kHeaderH));
        if (ImGui::IsItemActive()) {
            cat.posX += ImGui::GetIO().MouseDelta.x;
            cat.posY += ImGui::GetIO().MouseDelta.y;
        }
        bool headerHover = ImGui::IsItemHovered();
        // right-click header toggles expand (vape behavior)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            cat.expanded = !cat.expanded;

        // icon + title
        DrawIcon(cat.iconFile.c_str(), ImVec2(wp.x + 12, wp.y + 13), ImVec2(15, 15), ToU32(txt));
        dl->AddText(ImVec2(wp.x + 33, wp.y + 12), ToU32(txt), cat.name.c_str());

        if (!editMode) {
            // pencil (visible on header hover, like vape). The hover flag keeps
            // it visible while a click is held down (e.g. toggling expand via
            // the arrow / right-click): default IsWindowHovered() returns false
            // whenever any item is active, which made the pencil blink out for
            // the duration of every toggle click.
            if (headerHover ||
                ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                ImGui::SetCursorPos(ImVec2(kPanelW - 49, 0));
                std::string pid = "##pencil_" + cat.name;
                ImGui::InvisibleButton(pid.c_str(), ImVec2(20, 40));
                bool ph = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked()) editMode = true;
                DrawIcon("editlarge.png", ImVec2(wp.x + kPanelW - 45, wp.y + 14), ImVec2(12, 12),
                         ToU32(ph ? ImVec4(220.0f/255,220.0f/255,220.0f/255,1) : gray));
            }
            // arrow
            ImGui::SetCursorPos(ImVec2(kPanelW - 29, 0));
            std::string aid = "##arrow_" + cat.name;
            ImGui::InvisibleButton(aid.c_str(), ImVec2(27, 40));
            bool ah = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                cat.expanded = !cat.expanded;
            ImVec2 ap0 = ImVec2(wp.x + kPanelW - 20, wp.y + 18);
            // One glyph only (reference: single arrow ImageLabel with
            // Rotation 0/180). The icon is flipped via UVs when collapsed;
            // the triangle is purely a missing-asset fallback so the two can
            // never overlap pointing opposite directions.
            ImU32 acol = ToU32(ah ? ImVec4(220.0f/255,220.0f/255,220.0f/255,1) : gray);
            auto ait = icons.find("downexpand.png");
            if (ait != icons.end() && ait->second.srv) {
                ImVec2 ip1 = ImVec2(ap0.x + 9, ap0.y + 4);
                if (cat.expanded)
                    dl->AddImage((ImTextureID)ait->second.srv, ap0, ip1,
                                 ImVec2(0, 0), ImVec2(1, 1), acol);
                else
                    dl->AddImage((ImTextureID)ait->second.srv, ap0, ip1,
                                 ImVec2(0, 1), ImVec2(1, 0), acol);
            } else if (cat.expanded)
                dl->AddTriangleFilled(ImVec2(ap0.x, ap0.y), ImVec2(ap0.x + 9, ap0.y), ImVec2(ap0.x + 4.5f, ap0.y + 4.5f), acol);
            else
                dl->AddTriangleFilled(ImVec2(ap0.x, ap0.y + 4.5f), ImVec2(ap0.x + 9, ap0.y + 4.5f), ImVec2(ap0.x + 4.5f, ap0.y), acol);
        } else {
            // DONE button
            ImGui::SetCursorPos(ImVec2(kPanelW - 73, 0));
            ImGui::InvisibleButton(("##done_" + cat.name).c_str(), ImVec2(42, 40));
            bool dh = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) editMode = false;
            dl->AddText(ImVec2(wp.x + kPanelW - 68, wp.y + 12),
                        ToU32(dh ? ImVec4(220.0f/255,220.0f/255,220.0f/255,1) : gray), "DONE");
        }

        if (cat.expanded) {
            // divider under the header - reference puts it at y=37 (Category.lua)
            dl->AddRectFilled(ImVec2(wp.x, wp.y + kRowsTop), ImVec2(wp.x + kPanelW, wp.y + kRowsTop + 1),
                              IM_COL32(255, 255, 255, 18));
            // list region: y=37 .. winH-kBotGap, full-bleed (0px inset) so rows
            // reach the panel edge
            ImGui::SetCursorPos(ImVec2(0, kRowsTop));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // flush rows, like vape
            std::string childId = "##list_" + cat.name;
            ImGui::BeginChild(childId.c_str(), ImVec2(kPanelW, winH - kRowsTop - kBotGap), false,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar);
            ImDrawList* cdl = ImGui::GetWindowDrawList();
            float unused = 0.0f;
            for (auto& m : cat.modules) {
                if (!m.visible && !editMode) continue;
                float y0 = ImGui::GetCursorPosY();
                (void)y0;
                RenderModuleRow(cat, m, accentRgb, t, 0.0f, unused);
                (void)cdl;
            }
            // UIStroke drawn over the rows: the child's clip only covers the list
            // region, so widen it back to the panel rect for the stroke pass.
            cdl->PushClipRect(ImVec2(wp.x, wp.y), ImVec2(wp.x + kPanelW, wp.y + winH), false);
            FrostedPanelStroke(cdl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), kRadius);
            cdl->PopClipRect();
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    hitRects.push_back(HitRect{cat.posX, cat.posY, cat.posX + kPanelW, cat.posY + std::max(winH, kHeaderH)});
}
