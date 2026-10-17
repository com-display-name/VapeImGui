#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::RenderTextGui() {
    if (!TextGuiEnabled)
        return;
    struct Entry {
        std::string Name;
        ImVec4 Color;
    };
    std::vector<Entry> Entries;
    float Time = (float)ImGui::GetTime();
    for (auto& Category : Categories)
        for (auto& Module : Category.Modules)
            if (Module.Enabled && Module.Visible)
                Entries.push_back({Module.Name, GetAccentColor(Accent, Time)});
    if (Entries.empty())
        return;
    std::sort(Entries.begin(), Entries.end(), [](const Entry& A, const Entry& B) { return A.Name < B.Name; });

    ImDrawList* DrawList = ImGui::GetForegroundDrawList();
    ImGuiIO& Io = ImGui::GetIO();
    ImFont* Font = ImGui::GetFont();
    float FontSize = ImGui::GetFontSize();
    float X = Io.DisplaySize.x - 10.0f;
    float Y = 10.0f;
    for (auto& Item : Entries) {
        ImVec2 TextSize = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Item.Name.c_str());
        float Width = TextSize.x + 26.0f;
        float Height = 22.0f;
        FillPanel(DrawList, ImVec2(X - Width, Y), ImVec2(X, Y + Height), IM_COL32(10, 10, 10, 190), 4.0f);
        DrawList->AddRectFilled(ImVec2(X - Width, Y + 2), ImVec2(X - Width + 3, Y + Height - 2),
                                ToU32(Item.Color), 1.0f);
        DrawList->AddText(ImVec2(X - Width + 9, Y + (Height - TextSize.y) * 0.5f),
                          IM_COL32(255, 255, 255, 255), Item.Name.c_str());
        Y += Height + 3.0f;
    }
}

void VapeGui::RenderNotifications() {
    float Time = ElapsedSeconds();
    ImDrawList* DrawList = ImGui::GetForegroundDrawList();
    ImGuiIO& Io = ImGui::GetIO();
    ImFont* Font = ImGui::GetFont();
    float FontSize = ImGui::GetFontSize();

    Notifications.erase(std::remove_if(Notifications.begin(), Notifications.end(),
                                       [Time](const Vape::Notification& Notification) {
                                           return Time - Notification.Created > Notification.Duration + 0.4f;
                                       }),
                        Notifications.end());

    float Y = Io.DisplaySize.y - 20.0f;
    for (int I = (int)Notifications.size() - 1; I >= 0; --I) {
        const auto& Notification = Notifications[I];
        float Age = Time - Notification.Created;
        float Slide = std::min(Age / 0.4f, 1.0f);
        Slide = 1.0f - (1.0f - Slide) * (1.0f - Slide);

        // Translate instead of fading against the black colorkey.
        float ExitProgress =
            Age > Notification.Duration ? std::min((Age - Notification.Duration) / 0.35f, 1.0f) : 0.0f;
        ExitProgress = ExitProgress * ExitProgress * (3.0f - 2.0f * ExitProgress);

        ImVec2 TitleSize = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Notification.Title.c_str());
        ImVec2 TextSize = Font->CalcTextSizeA(FontSize * 0.92f, FLT_MAX, 0.0f, Notification.Text.c_str());
        float Width = std::max({TitleSize.x, TextSize.x}) + 52.0f;
        Width = std::max(Width, 170.0f);
        float Height = 60.0f;
        float X1 = Io.DisplaySize.x - 10.0f - Width * Slide + (Width + 20.0f) * ExitProgress;
        float Y1 = Y - Height;
        if (Y1 < 10)
            break;

        DrawPanel(DrawList, ImVec2(X1, Y1), ImVec2(X1 + Width, Y1 + Height), IM_COL32(18, 18, 18, 245), 8.0f);

        ImVec2 IconMin = ImVec2(X1 + 9, Y1 + 11);
        const char* IconName =
            Notification.Kind == Vape::NotificationKind::Alert ? "noti_alert.png" : "noti_info.png";
        auto Icon = Icons.find(IconName);
        if (Icon != Icons.end() && Icon->second.ShaderResource) {
            float Scale = Notification.Kind == Vape::NotificationKind::Alert ? 18.0f / 30.0f : 18.0f / 24.0f;
            float BadgeX = Notification.Kind == Vape::NotificationKind::Alert ? 46.0f : 48.0f;
            float BadgeY = Notification.Kind == Vape::NotificationKind::Alert ? 43.0f : 48.0f;
            float BadgeWidth = Notification.Kind == Vape::NotificationKind::Alert ? 33.0f : 24.0f;
            float BadgeHeight = Notification.Kind == Vape::NotificationKind::Alert ? 30.0f : 24.0f;
            float CanvasWidth = Notification.Kind == Vape::NotificationKind::Alert ? 126.0f : 120.0f;
            float CanvasHeight = Notification.Kind == Vape::NotificationKind::Alert ? 116.0f : 120.0f;

            ImVec2 IconMax = ImVec2(IconMin.x + BadgeWidth * Scale, IconMin.y + BadgeHeight * Scale);

            DrawList->PushClipRect(ImVec2(X1, Y1), ImVec2(X1 + Width, Y1 + Height), true);
            DrawList->AddImage(
                (ImTextureID)Icon->second.ShaderResource, IconMin, IconMax,
                ImVec2(BadgeX / CanvasWidth, BadgeY / CanvasHeight),
                ImVec2((BadgeX + BadgeWidth) / CanvasWidth, (BadgeY + BadgeHeight) / CanvasHeight),
                IM_COL32(255, 255, 255, 255));
            DrawList->PopClipRect();
        } else {
            DrawList->AddCircleFilled(ImVec2(IconMin.x + 9, IconMin.y + 9), 9.0f,
                                      IM_COL32(255, 255, 255, 255));
            DrawList->AddText(ImVec2(IconMin.x + 6.5f, IconMin.y + 2), IM_COL32(0, 0, 0, 255), "i");
        }
        ImU32 TitleColor = Notification.Kind == Vape::NotificationKind::Alert ? IM_COL32(250, 50, 56, 255)
                                                                              : IM_COL32(255, 255, 255, 255);
        DrawList->AddText(ImVec2(X1 + 37, Y1 + 11), TitleColor, Notification.Title.c_str());

        ImU32 SubtitleColor = IM_COL32(170, 170, 170, 255);
        if (Notification.Kind == Vape::NotificationKind::Alert ||
            Notification.Kind == Vape::NotificationKind::Warning)
            SubtitleColor = IM_COL32(250, 50, 56, 255);
        else if (Notification.Text == "Enabled")
            SubtitleColor = IM_COL32(90, 210, 130, 255);
        DrawList->AddText(Font, FontSize * 0.92f, ImVec2(X1 + 37, Y1 + 32), SubtitleColor,
                          Notification.Text.c_str());

        float Fraction = std::clamp(1.0f - Age / Notification.Duration, 0.0f, 1.0f);
        ImU32 ProgressColor = Notification.Kind == Vape::NotificationKind::Alert
                                  ? IM_COL32(250, 50, 56, 255)
                                  : IM_COL32(255, 255, 255, 255);
        DrawList->AddRectFilled(ImVec2(X1 + 8, Y1 + Height - 3),
                                ImVec2(X1 + 8 + (Width - 16) * Fraction, Y1 + Height), ProgressColor);
        Y = Y1 - 8.0f;
    }
}
