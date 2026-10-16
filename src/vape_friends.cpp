// Friends list (CategoryList.lua): player entries, add box, per-entry colour
// and the side options pane.
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

// ============================= Friends list (CategoryList.lua) =============================
ImVec2 VapeGui::FriendsSize() const {
    using namespace vape;
    float contentH = 31.0f; // add box
    for (size_t i = 0; i < friends.size(); ++i)
        contentH += 4.0f + 31.0f; // 4px gaps, 200x31 entries
    float winH = friendsExpanded ? std::min(45.0f + contentH + kBotGap, 611.0f) : 45.0f;
    return ImVec2(kPanelW, winH);
}

void VapeGui::VapeFriendHueSlider() {
    using namespace vape;
    ImVec4 dim = Darken(TextColor(), 0.16f);
    ImVec4 col = HSVtoRGB(friendsHue, friendsSat, friendsVal);
    float w = RowWidth(true);
    ImGui::InvisibleButton("##friend_hue", ImVec2(w, 50));
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    bool active = ImGui::IsItemActive();
    ImDrawList* d2 = ImGui::GetWindowDrawList();
    d2->AddRectFilled(r0, r1, ToU32(MainColor()));
    d2->AddText(ImVec2(r0.x + 10, r0.y + 4), ToU32(dim), "Friends color");
    ImVec2 t0 = ImVec2(r0.x + 10, r0.y + 37);
    ImVec2 t1 = ImVec2(r1.x - 10, r0.y + 39);
    const int segs = 24;
    for (int i = 0; i < segs; ++i) {
        float f0 = (float)i / segs, f1 = (float)(i + 1) / segs;
        d2->AddRectFilled(ImVec2(t0.x + (t1.x - t0.x) * f0, t0.y),
                          ImVec2(t0.x + (t1.x - t0.x) * f1, t1.y),
                          ToU32(HSVtoRGB(f0, 1, 1)));
    }
    float frac = std::clamp(friendsHue, 0.04f, 0.96f);
    float fx = t0.x + (t1.x - t0.x) * frac;
    d2->AddCircleFilled(ImVec2(fx, (t0.y + t1.y) * 0.5f), 7.0f, ToU32(col));
    ImVec2 mp = ImGui::GetIO().MousePos;
    if (active && mp.y > r0.y + 20)
        friendsHue = std::clamp((mp.x - t0.x) / (t1.x - t0.x), 0.0f, 1.0f);
}

void VapeGui::RenderFriends() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);
    (void)t;
    using namespace vape;
    ImVec4 bg = MainColor();
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImVec4 gray = ImVec4(140.0f / 255, 140.0f / 255, 140.0f / 255, 1);
    ImVec4 friendCol = HSVtoRGB(friendsHue, friendsSat, friendsVal);

    float winH = FriendsSize().y;

    ImGui::SetNextWindowPos(ImVec2(friendsX, friendsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, std::max(winH, 45.0f)), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    bool open = ImGui::Begin("##VapeFriends", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
    ImVec2 wp = ImGui::GetWindowPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (open) {
        FrostedPanel(dl, wp, ImVec2(wp.x + kPanelW, wp.y + winH), ToU32(bg), kRadius);

        // header drag (exclude right 70px reserved for gear/arrow)
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##friends_drag", ImVec2(kPanelW - 70, kHeaderH));
        if (ImGui::IsItemActive()) {
            friendsX += ImGui::GetIO().MouseDelta.x;
            friendsY += ImGui::GetIO().MouseDelta.y;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            friendsExpanded = !friendsExpanded;

        DrawIcon("friends.png", ImVec2(wp.x + 12, wp.y + 12), ImVec2(17, 16), ToU32(txt));
        dl->AddText(ImVec2(wp.x + 36, wp.y + 12), ToU32(txt), "Friends");

        // settings gear: opens the side options pane
        ImGui::SetCursorPos(ImVec2(kPanelW - 56, 15));
        ImGui::InvisibleButton("##friends_opts", ImVec2(14, 14));
        bool shov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            friendsOptionsOpen = !friendsOptionsOpen;
        DrawIcon("settings.png", ImVec2(wp.x + kPanelW - 56, wp.y + 15), ImVec2(14, 14),
                 ToU32(shov ? txt : Darken(txt, 0.43f)));
        if (shov) ShowTooltip("Friends options");

        // expand arrow
        ImGui::SetCursorPos(ImVec2(kPanelW - 40, 0));
        ImGui::InvisibleButton("##friends_arrow", ImVec2(40, 40));
        bool ah = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
            friendsExpanded = !friendsExpanded;
        ImVec2 ap0 = ImVec2(wp.x + kPanelW - 25, wp.y + 20);
        ImU32 acol = ToU32(ah ? ImVec4(220.0f / 255, 220.0f / 255, 220.0f / 255, 1) : gray);
        auto ait = icons.find("downexpand.png");
        if (ait != icons.end() && ait->second.srv) {
            ImVec2 ip1 = ImVec2(ap0.x + 9, ap0.y + 4);
            if (friendsExpanded)
                dl->AddImage((ImTextureID)ait->second.srv, ap0, ip1,
                             ImVec2(0, 0), ImVec2(1, 1), acol);
            else
                dl->AddImage((ImTextureID)ait->second.srv, ap0, ip1,
                             ImVec2(0, 1), ImVec2(1, 0), acol);
        } else if (friendsExpanded)
            dl->AddTriangleFilled(ImVec2(ap0.x, ap0.y), ImVec2(ap0.x + 9, ap0.y),
                                  ImVec2(ap0.x + 4.5f, ap0.y + 4.5f), acol);
        else
            dl->AddTriangleFilled(ImVec2(ap0.x, ap0.y + 4.5f), ImVec2(ap0.x + 9, ap0.y + 4.5f),
                                  ImVec2(ap0.x + 4.5f, ap0.y), acol);

        if (friendsExpanded && winH > 45.0f) {
            dl->AddRectFilled(ImVec2(wp.x, wp.y + 41), ImVec2(wp.x + kPanelW, wp.y + 42),
                              IM_COL32(255, 255, 255, 18));
            ImGui::SetCursorPos(ImVec2(0, 45));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::BeginChild("##friends_list", ImVec2(kPanelW, winH - 45 - kBotGap), false,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar);
            ImDrawList* cdl = ImGui::GetWindowDrawList();
            ImVec2 co = ImGui::GetCursorScreenPos();
            float y = co.y;

            // add box: 200x31 outer + 1px dark inner, textbox + add button
            {
                ImVec2 ab0(co.x + 10, y), ab1(ab0.x + 200, y + 31);
                ImVec2 mp = ImGui::GetIO().MousePos;
                bool abHov = mp.x >= ab0.x && mp.x <= ab1.x && mp.y >= ab0.y && mp.y <= ab1.y &&
                             ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                ImGui::SetCursorScreenPos(ab0);
                ImGui::Dummy(ImVec2(200, 31));
                cdl->AddRectFilled(ab0, ab1, ToU32(abHov ? Lighten(MainColor(), 0.14f)
                                                         : Lighten(MainColor(), 0.02f)), 5.0f);
                cdl->AddRectFilled(ImVec2(ab0.x + 1, ab0.y + 1), ImVec2(ab1.x - 1, ab1.y - 1),
                                   ToU32(Darken(MainColor(), 0.02f)), 5.0f);
                ImGui::SetCursorScreenPos(ImVec2(ab0.x + 10, ab0.y));
                ImGui::PushItemWidth(200 - 45);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f)));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 9));
                bool enter = ImGui::InputTextWithHint("##friend_add", "Roblox username",
                                                      friendBuf, IM_ARRAYSIZE(friendBuf),
                                                      ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
                ImGui::PopItemWidth();
                auto addFriend = [&]() {
                    if (friendBuf[0] == '\0') return;
                    for (auto& f : friends)
                        if (f.name == friendBuf) { friendBuf[0] = '\0'; return; }
                    FriendEntry e; e.name = friendBuf; e.enabled = true;
                    friends.push_back(e);
                    friendBuf[0] = '\0';
                };
                if (enter) addFriend();
                ImVec2 pb(ab1.x - 26, ab0.y + 8);
                bool phov = mp.x >= pb.x && mp.x <= pb.x + 16 && mp.y >= pb.y && mp.y <= pb.y + 16 &&
                            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
                DrawIcon("add.png", pb, ImVec2(16, 16),
                         ToU32(ImVec4(friendCol.x, friendCol.y, friendCol.z, phov ? 1.0f : 0.7f)));
                if (phov && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) addFriend();
                y += 31 + 4;
            }

            // entries: 200x31 rows with enable dot + name + remove button
            for (size_t i = 0; i < friends.size(); ++i) {
                auto& e = friends[i];
                ImGui::SetCursorScreenPos(ImVec2(co.x + 10, y));
                std::string eid = "##friend_" + std::to_string(i);
                ImGui::InvisibleButton(eid.c_str(), ImVec2(200, 31));
                bool ehov = ImGui::IsItemHovered();
                ImVec2 e0 = ImGui::GetItemRectMin(), e1 = ImGui::GetItemRectMax();
                ImVec2 mp = ImGui::GetIO().MousePos;
                bool inClose = mp.x >= e1.x - 27 && mp.x <= e1.x && mp.y >= e0.y && mp.y <= e1.y;
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    if (inClose) {
                        friends.erase(friends.begin() + (long)i);
                        --i;
                        y -= 31 + 4;
                        continue;
                    }
                    e.enabled = !e.enabled;
                }
                cdl->AddRectFilled(e0, e1, ToU32(Lighten(MainColor(), 0.02f)), 5.0f);
                if (ehov)
                    cdl->AddRectFilled(ImVec2(e0.x + 1, e0.y + 1), ImVec2(e1.x - 1, e1.y - 1),
                                       ToU32(MainColor()), 5.0f);
                ImVec2 dot0(e0.x + 10, e0.y + 12);
                ImVec4 dotOn = friendsRecolor ? friendCol
                                              : ImVec4(5.0f / 255, 134.0f / 255, 105.0f / 255, 1);
                cdl->AddRectFilled(dot0, ImVec2(dot0.x + 10, dot0.y + 11),
                                   ToU32(e.enabled ? dotOn : Lighten(MainColor(), 0.37f)), 5.5f);
                cdl->AddRectFilled(ImVec2(dot0.x + 1, dot0.y + 1), ImVec2(dot0.x + 9, dot0.y + 10),
                                   ToU32(e.enabled ? dotOn : Lighten(MainColor(), 0.02f)), 4.5f);
                cdl->AddText(ImVec2(e0.x + 30, e0.y + 7), ToU32(Darken(txt, 0.16f)), e.name.c_str());
                ImVec2 cb(e1.x - 27, e0.y + 8);
                bool chov = ehov && inClose;
                if (chov)
                    cdl->AddRectFilled(cb, ImVec2(cb.x + 18, cb.y + 17),
                                       IM_COL32(255, 255, 255, 102), 9.0f);
                ImVec4 closeBase = Lighten(MainColor(), 0.37f);
                DrawIcon("closetiny.png", cb, ImVec2(18, 17),
                         ToU32(ImVec4(closeBase.x, closeBase.y, closeBase.z, chov ? 0.7f : 0.5f)));
                y += 31 + 4;
            }

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

    hitRects.push_back(HitRect{friendsX, friendsY, friendsX + kPanelW, friendsY + std::max(winH, 45.0f)});

    // Side options pane, attached to the right of the list (childrentwo).
    if (friendsOptionsOpen && friendsExpanded) {
        float ox = friendsX + kPanelW + 4.0f;
        float ow = kPanelW;
        float oh = 41.0f + 30.0f + 50.0f + 30.0f + kBotGap;
        ImGui::SetNextWindowPos(ImVec2(ox, friendsY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(ow, oh), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        bool oopen = ImGui::Begin("##VapeFriendsOptions", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
        ImVec2 op = ImGui::GetWindowPos();
        ImDrawList* odl = ImGui::GetWindowDrawList();
        if (oopen) {
            FrostedPanel(odl, op, ImVec2(op.x + ow, op.y + oh), ToU32(bg), kRadius);
            odl->AddText(ImVec2(op.x + 14, op.y + 12), ToU32(txt), "Options");
            odl->AddRectFilled(ImVec2(op.x, op.y + 41), ImVec2(op.x + ow, op.y + 42),
                               IM_COL32(255, 255, 255, 18));
            ImGui::SetCursorPos(ImVec2(0, 41));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::BeginChild("##friends_opts_list", ImVec2(ow, oh - 49), false,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar);
            ImDrawList* od2 = ImGui::GetWindowDrawList();
            VapeToggleRow("Recolor visuals", friendsRecolor, "##f_recolor", accentRgb);
            VapeFriendHueSlider();
            VapeToggleRow("Use friends", friendsUse, "##f_use", accentRgb);
            od2->PushClipRect(op, ImVec2(op.x + ow, op.y + oh), false);
            FrostedPanelStroke(od2, op, ImVec2(op.x + ow, op.y + oh), kRadius);
            od2->PopClipRect();
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        hitRects.push_back(HitRect{ox, friendsY, ox + ow, friendsY + oh});
    }
}
