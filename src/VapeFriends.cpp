#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

ImVec2 VapeGui::GetFriendsSize() const {
    using namespace Vape;
    float ContentHeight = 31.0f;
    for (size_t I = 0; I < Friends.size(); ++I)
        ContentHeight += 4.0f + 31.0f;
    float WindowHeight = FriendsExpanded ? std::min(45.0f + ContentHeight + BottomGap, 611.0f) : 45.0f;
    return ImVec2(PanelWidth, WindowHeight);
}

void VapeGui::RenderFriendHueSlider() {
    using namespace Vape;
    ImVec4 DimText = Darken(TextColor(), 0.16f);
    ImVec4 Color = HsvToRgb(FriendsHue, FriendsSaturation, FriendsValue);
    float Width = RowWidth();
    ImGui::InvisibleButton("##friend_hue", ImVec2(Width, 50));
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    bool Active = ImGui::IsItemActive();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(MainColor()));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 4), ToU32(DimText), "Friends color");
    ImVec2 TrackMin = ImVec2(RowMin.x + 10, RowMin.y + 37);
    ImVec2 TrackMax = ImVec2(RowMax.x - 10, RowMin.y + 39);
    const int Segs = 24;
    for (int I = 0; I < Segs; ++I) {
        float F0 = (float)I / Segs, F1 = (float)(I + 1) / Segs;
        DrawList->AddRectFilled(ImVec2(TrackMin.x + (TrackMax.x - TrackMin.x) * F0, TrackMin.y),
                                ImVec2(TrackMin.x + (TrackMax.x - TrackMin.x) * F1, TrackMax.y),
                                ToU32(HsvToRgb(F0, 1, 1)));
    }
    float Fraction = std::clamp(FriendsHue, 0.04f, 0.96f);
    float FillX = TrackMin.x + (TrackMax.x - TrackMin.x) * Fraction;
    DrawList->AddCircleFilled(ImVec2(FillX, (TrackMin.y + TrackMax.y) * 0.5f), 7.0f, ToU32(Color));
    ImVec2 MousePosition = ImGui::GetIO().MousePos;
    if (Active && MousePosition.y > RowMin.y + 20)
        FriendsHue = std::clamp((MousePosition.x - TrackMin.x) / (TrackMax.x - TrackMin.x), 0.0f, 1.0f);
}

void VapeGui::RenderFriends() {
    float Time = (float)ImGui::GetTime();
    ImVec4 AccentColor = GetAccentColor(Accent, Time);

    using namespace Vape;
    ImVec4 Background = MainColor();
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImVec4 Gray = ImVec4(140.0f / 255, 140.0f / 255, 140.0f / 255, 1);
    ImVec4 FriendColor = HsvToRgb(FriendsHue, FriendsSaturation, FriendsValue);

    float WindowHeight = GetFriendsSize().y;

    bool Open = BeginPanel("##VapeFriends", ImVec2(FriendsX, FriendsY),
                           ImVec2(PanelWidth, std::max(WindowHeight, 45.0f)));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + WindowHeight), ToU32(Background),
                  PanelRadius);

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##friends_drag", ImVec2(PanelWidth - 70, HeaderHeight));
        if (ImGui::IsItemActive()) {
            FriendsX += ImGui::GetIO().MouseDelta.x;
            FriendsY += ImGui::GetIO().MouseDelta.y;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            FriendsExpanded = !FriendsExpanded;

        DrawIcon("friends.png", ImVec2(WindowPosition.x + 12, WindowPosition.y + 12), ImVec2(17, 16),
                 ToU32(Text));
        DrawList->AddText(ImVec2(WindowPosition.x + 36, WindowPosition.y + 12), ToU32(Text), "Friends");

        ImGui::SetCursorPos(ImVec2(PanelWidth - 56, 15));
        ImGui::InvisibleButton("##friends_opts", ImVec2(14, 14));
        bool SettingsHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            FriendsOptionsOpen = !FriendsOptionsOpen;
        DrawIcon("settings.png", ImVec2(WindowPosition.x + PanelWidth - 56, WindowPosition.y + 15),
                 ImVec2(14, 14), ToU32(SettingsHovered ? Text : Darken(Text, 0.43f)));
        if (SettingsHovered)
            ShowTooltip("Friends options");

        ImGui::SetCursorPos(ImVec2(PanelWidth - 40, 0));
        ImGui::InvisibleButton("##friends_arrow", ImVec2(40, 40));
        bool ArrowHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
            FriendsExpanded = !FriendsExpanded;
        ImVec2 ArrowPosition = ImVec2(WindowPosition.x + PanelWidth - 25, WindowPosition.y + 20);
        ImU32 ArrowColor = ToU32(ArrowHovered ? ImVec4(220.0f / 255, 220.0f / 255, 220.0f / 255, 1) : Gray);
        auto ArrowIcon = Icons.find("downexpand.png");
        if (ArrowIcon != Icons.end() && ArrowIcon->second.ShaderResource) {
            ImVec2 IconMax = ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4);
            if (FriendsExpanded)
                DrawList->AddImage((ImTextureID)ArrowIcon->second.ShaderResource, ArrowPosition, IconMax,
                                   ImVec2(0, 0), ImVec2(1, 1), ArrowColor);
            else
                DrawList->AddImage((ImTextureID)ArrowIcon->second.ShaderResource, ArrowPosition, IconMax,
                                   ImVec2(0, 1), ImVec2(1, 0), ArrowColor);
        } else if (FriendsExpanded)
            DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y),
                                        ImVec2(ArrowPosition.x + 9, ArrowPosition.y),
                                        ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y + 4.5f), ArrowColor);
        else
            DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y + 4.5f),
                                        ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4.5f),
                                        ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y), ArrowColor);

        if (FriendsExpanded && WindowHeight > 45.0f) {
            DrawList->AddRectFilled(ImVec2(WindowPosition.x, WindowPosition.y + 41),
                                    ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + 42),
                                    IM_COL32(255, 255, 255, 18));
            ImGui::SetCursorPos(ImVec2(0, 45));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            BeginScrollableChild("##friends_list", ImVec2(PanelWidth, WindowHeight - 45 - BottomGap));
            ImDrawList* ContentDrawList = ImGui::GetWindowDrawList();
            ImVec2 ContentPosition = ImGui::GetCursorScreenPos();
            float Y = ContentPosition.y;

            {
                ImVec2 AddBoxMin(ContentPosition.x + 10, Y), AddBoxMax(AddBoxMin.x + 200, Y + 31);
                ImVec2 MousePosition = ImGui::GetIO().MousePos;
                bool AddBoxHovered = MousePosition.x >= AddBoxMin.x && MousePosition.x <= AddBoxMax.x &&
                                     MousePosition.y >= AddBoxMin.y && MousePosition.y <= AddBoxMax.y &&
                                     ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                ImGui::SetCursorScreenPos(AddBoxMin);
                ImGui::Dummy(ImVec2(200, 31));
                ContentDrawList->AddRectFilled(
                    AddBoxMin, AddBoxMax,
                    ToU32(AddBoxHovered ? Lighten(MainColor(), 0.14f) : Lighten(MainColor(), 0.02f)), 5.0f);
                ContentDrawList->AddRectFilled(ImVec2(AddBoxMin.x + 1, AddBoxMin.y + 1),
                                               ImVec2(AddBoxMax.x - 1, AddBoxMax.y - 1),
                                               ToU32(Darken(MainColor(), 0.02f)), 5.0f);
                ImGui::SetCursorScreenPos(ImVec2(AddBoxMin.x + 10, AddBoxMin.y));
                ImGui::PushItemWidth(200 - 45);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f)));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 9));
                bool Enter = ImGui::InputTextWithHint("##friend_add", "Roblox username", FriendNameBuffer,
                                                      IM_ARRAYSIZE(FriendNameBuffer),
                                                      ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
                ImGui::PopItemWidth();
                auto AddFriend = [&]() {
                    if (FriendNameBuffer[0] == '\0')
                        return;
                    for (auto& Friend : Friends)
                        if (Friend.Name == FriendNameBuffer) {
                            FriendNameBuffer[0] = '\0';
                            return;
                        }
                    FriendEntry Entry;
                    Entry.Name = FriendNameBuffer;
                    Entry.Enabled = true;
                    Friends.push_back(Entry);
                    FriendNameBuffer[0] = '\0';
                };
                if (Enter)
                    AddFriend();
                ImVec2 AddPosition(AddBoxMax.x - 26, AddBoxMin.y + 8);
                bool AddButtonHovered =
                    MousePosition.x >= AddPosition.x && MousePosition.x <= AddPosition.x + 16 &&
                    MousePosition.y >= AddPosition.y && MousePosition.y <= AddPosition.y + 16 &&
                    ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                DrawIcon("add.png", AddPosition, ImVec2(16, 16),
                         ToU32(ImVec4(FriendColor.x, FriendColor.y, FriendColor.z,
                                      AddButtonHovered ? 1.0f : 0.7f)));
                if (AddButtonHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    AddFriend();
                Y += 31 + 4;
            }

            for (size_t I = 0; I < Friends.size(); ++I) {
                auto& Entry = Friends[I];
                ImGui::SetCursorScreenPos(ImVec2(ContentPosition.x + 10, Y));
                std::string EntryId = "##friend_" + std::to_string(I);
                ImGui::InvisibleButton(EntryId.c_str(), ImVec2(200, 31));
                bool EntryHovered = ImGui::IsItemHovered();
                ImVec2 EntryMin = ImGui::GetItemRectMin(), EntryMax = ImGui::GetItemRectMax();
                ImVec2 MousePosition = ImGui::GetIO().MousePos;
                bool InClose = MousePosition.x >= EntryMax.x - 27 && MousePosition.x <= EntryMax.x &&
                               MousePosition.y >= EntryMin.y && MousePosition.y <= EntryMax.y;
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (InClose) {
                        Friends.erase(Friends.begin() + (long)I);
                        --I;
                        Y -= 31 + 4;
                        continue;
                    }
                    Entry.Enabled = !Entry.Enabled;
                }
                ContentDrawList->AddRectFilled(EntryMin, EntryMax, ToU32(Lighten(MainColor(), 0.02f)), 5.0f);
                if (EntryHovered)
                    ContentDrawList->AddRectFilled(ImVec2(EntryMin.x + 1, EntryMin.y + 1),
                                                   ImVec2(EntryMax.x - 1, EntryMax.y - 1), ToU32(MainColor()),
                                                   5.0f);
                ImVec2 Dot0(EntryMin.x + 10, EntryMin.y + 12);
                ImVec4 DotOn =
                    FriendsRecolor ? FriendColor : ImVec4(5.0f / 255, 134.0f / 255, 105.0f / 255, 1);
                ContentDrawList->AddRectFilled(Dot0, ImVec2(Dot0.x + 10, Dot0.y + 11),
                                               ToU32(Entry.Enabled ? DotOn : Lighten(MainColor(), 0.37f)),
                                               5.5f);
                ContentDrawList->AddRectFilled(
                    ImVec2(Dot0.x + 1, Dot0.y + 1), ImVec2(Dot0.x + 9, Dot0.y + 10),
                    ToU32(Entry.Enabled ? DotOn : Lighten(MainColor(), 0.02f)), 4.5f);
                ContentDrawList->AddText(ImVec2(EntryMin.x + 30, EntryMin.y + 7), ToU32(Darken(Text, 0.16f)),
                                         Entry.Name.c_str());
                ImVec2 ClosePosition(EntryMax.x - 27, EntryMin.y + 8);
                bool CloseHovered = EntryHovered && InClose;
                if (CloseHovered)
                    ContentDrawList->AddRectFilled(ClosePosition,
                                                   ImVec2(ClosePosition.x + 18, ClosePosition.y + 17),
                                                   IM_COL32(255, 255, 255, 102), 9.0f);
                ImVec4 CloseBase = Lighten(MainColor(), 0.37f);
                DrawIcon("closetiny.png", ClosePosition, ImVec2(18, 17),
                         ToU32(ImVec4(CloseBase.x, CloseBase.y, CloseBase.z, CloseHovered ? 0.7f : 0.5f)));
                Y += 31 + 4;
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

    HitRects.push_back(
        HitRect{FriendsX, FriendsY, FriendsX + PanelWidth, FriendsY + std::max(WindowHeight, 45.0f)});

    if (FriendsOptionsOpen && FriendsExpanded) {
        float OptionsX = FriendsX + PanelWidth + 4.0f;
        float OptionsWidth = PanelWidth;
        float OptionsHeight = 41.0f + 30.0f + 50.0f + 30.0f + BottomGap;
        bool OptionsVisible = BeginPanel("##VapeFriendsOptions", ImVec2(OptionsX, FriendsY),
                                         ImVec2(OptionsWidth, OptionsHeight));
        ImVec2 OptionsPosition = ImGui::GetWindowPos();
        ImDrawList* OptionsDrawList = ImGui::GetWindowDrawList();
        if (OptionsVisible) {
            DrawPanel(OptionsDrawList, OptionsPosition,
                      ImVec2(OptionsPosition.x + OptionsWidth, OptionsPosition.y + OptionsHeight),
                      ToU32(Background), PanelRadius);
            OptionsDrawList->AddText(ImVec2(OptionsPosition.x + 14, OptionsPosition.y + 12), ToU32(Text),
                                     "Options");
            OptionsDrawList->AddRectFilled(ImVec2(OptionsPosition.x, OptionsPosition.y + 41),
                                           ImVec2(OptionsPosition.x + OptionsWidth, OptionsPosition.y + 42),
                                           IM_COL32(255, 255, 255, 18));
            ImGui::SetCursorPos(ImVec2(0, 41));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            BeginScrollableChild("##friends_opts_list", ImVec2(OptionsWidth, OptionsHeight - 49));
            ImDrawList* OptionsContentDrawList = ImGui::GetWindowDrawList();
            RenderToggleRow("Recolor visuals", FriendsRecolor, "##f_recolor", AccentColor);
            RenderFriendHueSlider();
            RenderToggleRow("Use friends", FriendsUse, "##f_use", AccentColor);
            OptionsContentDrawList->PushClipRect(
                OptionsPosition, ImVec2(OptionsPosition.x + OptionsWidth, OptionsPosition.y + OptionsHeight),
                false);
            DrawPanelBorder(OptionsContentDrawList, OptionsPosition,
                            ImVec2(OptionsPosition.x + OptionsWidth, OptionsPosition.y + OptionsHeight),
                            PanelRadius);
            OptionsContentDrawList->PopClipRect();
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        EndPanel();

        HitRects.push_back(HitRect{OptionsX, FriendsY, OptionsX + OptionsWidth, FriendsY + OptionsHeight});
    }
}
