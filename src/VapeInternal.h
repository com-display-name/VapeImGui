#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <string>

#include "VapeStyle.h"

using Vape::HeaderHeight;
using Vape::LegitHeight;
using Vape::LegitWidth;
using Vape::ModuleHeight;
using Vape::PanelRadius;
using Vape::PanelWidth;
using Vape::SearchHeight;
using Vape::SearchTop;

inline ImU32 ToU32(const ImVec4& Color) {
    return IM_COL32((int)(Color.x * 255), (int)(Color.y * 255), (int)(Color.z * 255), (int)(Color.w * 255));
}

inline float ElapsedSeconds() {
    using namespace std::chrono;
    static const auto Start = steady_clock::now();
    return (float)duration_cast<milliseconds>(steady_clock::now() - Start).count() / 1000.0f;
}

inline int GetBindableKey(int Index) {
    static int Keys[52];
    static bool Initialized = false;
    if (!Initialized) {
        Initialized = true;
        int Count = 0;
        for (int Key = 0x41; Key <= 0x5A; ++Key)
            Keys[Count++] = Key;
        for (int Key = 0x30; Key <= 0x39; ++Key)
            Keys[Count++] = Key;
        for (int Key = 0x70; Key <= 0x7B; ++Key)
            Keys[Count++] = Key;
        Keys[Count++] = VK_SPACE;
        Keys[Count++] = VK_SHIFT;
        Keys[Count++] = VK_CONTROL;
        Keys[Count++] = VK_MENU;
    }
    return (Index >= 0 && Index < 52) ? Keys[Index] : 0;
}

inline std::string GetKeyName(int Key) {
    if (Key >= 0x41 && Key <= 0x5A)
        return std::string(1, (char)Key);
    if (Key >= 0x30 && Key <= 0x39)
        return std::string(1, (char)Key);
    if (Key >= 0x70 && Key <= 0x7B)
        return "F" + std::to_string(Key - 0x70 + 1);
    switch (Key) {
    case VK_SPACE:
        return "SPACE";
    case VK_SHIFT:
        return "SHIFT";
    case VK_CONTROL:
        return "CTRL";
    case VK_MENU:
        return "ALT";
    default:
        break;
    }
    return "";
}

constexpr float Pi = 3.14159265358979323846f;
constexpr int CornerSegments = 12;
constexpr float QuarterTurn = Pi * 0.5f;

inline float CornerAngle(float StartAngle, int SegmentIndex) {
    return StartAngle + ((float)SegmentIndex / (float)CornerSegments) * QuarterTurn;
}
inline void PanelCorners(ImVec2 Minimum, ImVec2 Maximum, float Radius, ImVec2 Centers[4], float Starts[4]) {
    Centers[0] = ImVec2(Maximum.x - Radius, Minimum.y + Radius);
    Starts[0] = -QuarterTurn;
    Centers[1] = ImVec2(Maximum.x - Radius, Maximum.y - Radius);
    Starts[1] = 0.0f;
    Centers[2] = ImVec2(Minimum.x + Radius, Maximum.y - Radius);
    Starts[2] = QuarterTurn;
    Centers[3] = ImVec2(Minimum.x + Radius, Minimum.y + Radius);
    Starts[3] = Pi;
}

// Overlapping triangulations prevent colorkey pinholes along shared edges.
inline void FillSolidRectangle(ImDrawList* DrawList, ImVec2 Minimum, ImVec2 Maximum, ImU32 Color) {
    if (Maximum.x <= Minimum.x || Maximum.y <= Minimum.y)
        return;
    const ImVec2 Rectangle[4] = {Minimum, ImVec2(Maximum.x, Minimum.y), Maximum,
                                 ImVec2(Minimum.x, Maximum.y)};
    DrawList->AddConvexPolyFilled(Rectangle, 4, Color);
    const float MiddleY = (Minimum.y + Maximum.y) * 0.5f;
    const ImVec2 TopHalf[4] = {Minimum, ImVec2(Maximum.x, Minimum.y), ImVec2(Maximum.x, MiddleY),
                               ImVec2(Minimum.x, MiddleY)};
    DrawList->AddConvexPolyFilled(TopHalf, 4, Color);
    const ImVec2 BottomHalf[4] = {ImVec2(Minimum.x, MiddleY), ImVec2(Maximum.x, MiddleY), Maximum,
                                  ImVec2(Minimum.x, Maximum.y)};
    DrawList->AddConvexPolyFilled(BottomHalf, 4, Color);
}

inline void FillSolidCorner(ImDrawList* DrawList, ImVec2 Center, float Radius, float StartAngle,
                            ImU32 Color) {
    ImVec2 Points[CornerSegments + 2];
    Points[0] = Center;
    for (int Index = 0; Index <= CornerSegments; ++Index) {
        const float Angle = CornerAngle(StartAngle, Index);
        Points[1 + Index] = ImVec2(Center.x + cosf(Angle) * Radius, Center.y + sinf(Angle) * Radius);
    }
    DrawList->AddConvexPolyFilled(Points, CornerSegments + 2, Color);
    const int AnchorIndex = 1 + CornerSegments / 4;
    ImVec2 RotatedPoints[CornerSegments + 2];
    for (int Index = 0; Index <= CornerSegments + 1; ++Index)
        RotatedPoints[Index] = Points[(AnchorIndex + Index <= CornerSegments + 1)
                                          ? (AnchorIndex + Index)
                                          : (AnchorIndex + Index - (CornerSegments + 2))];
    DrawList->AddConvexPolyFilled(RotatedPoints, CornerSegments + 2, Color);
}
// Antialiasing against the transparent clear color creates dark fringes.
inline void FillPanel(ImDrawList* DrawList, ImVec2 Minimum, ImVec2 Maximum, ImU32 Color, float Radius) {
    ImDrawListFlags SavedFlags = DrawList->Flags;
    DrawList->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);
    ImVec2 Centers[4];
    float Starts[4];
    PanelCorners(Minimum, Maximum, Radius, Centers, Starts);
    for (int CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
        FillSolidCorner(DrawList, Centers[CornerIndex], Radius, Starts[CornerIndex], Color);

    const float Overlap = (Radius * 0.5f < 1.0f) ? Radius * 0.5f : 1.0f;
    FillSolidRectangle(DrawList, ImVec2(Minimum.x + Radius - Overlap, Minimum.y + Radius - Overlap),
                       ImVec2(Maximum.x - Radius + Overlap, Maximum.y - Radius + Overlap), Color);
    FillSolidRectangle(DrawList, ImVec2(Minimum.x + Radius, Minimum.y),
                       ImVec2(Maximum.x - Radius, Minimum.y + Radius), Color);
    FillSolidRectangle(DrawList, ImVec2(Minimum.x + Radius, Maximum.y - Radius),
                       ImVec2(Maximum.x - Radius, Maximum.y), Color);
    FillSolidRectangle(DrawList, ImVec2(Minimum.x, Minimum.y + Radius),
                       ImVec2(Minimum.x + Radius, Maximum.y - Radius), Color);
    FillSolidRectangle(DrawList, ImVec2(Maximum.x - Radius, Minimum.y + Radius),
                       ImVec2(Maximum.x, Maximum.y - Radius), Color);
    DrawList->Flags = SavedFlags;
}

// Match the fill geometry exactly; overlap corner segments to avoid gaps.
inline void DrawPanelBorder(ImDrawList* DrawList, ImVec2 Minimum, ImVec2 Maximum, float Radius) {
    const ImU32 Color = IM_COL32(60, 60, 60, 255);
    ImDrawListFlags SavedFlags = DrawList->Flags;
    DrawList->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);

    FillSolidRectangle(DrawList, ImVec2(Minimum.x + Radius, Minimum.y),
                       ImVec2(Maximum.x - Radius, Minimum.y + 1.0f), Color);
    FillSolidRectangle(DrawList, ImVec2(Maximum.x - 1.0f, Minimum.y + Radius),
                       ImVec2(Maximum.x, Maximum.y - Radius), Color);
    FillSolidRectangle(DrawList, ImVec2(Minimum.x + Radius, Maximum.y - 1.0f),
                       ImVec2(Maximum.x - Radius, Maximum.y), Color);
    FillSolidRectangle(DrawList, ImVec2(Minimum.x, Minimum.y + Radius),
                       ImVec2(Minimum.x + 1.0f, Maximum.y - Radius), Color);

    const float InnerRadius = Radius - 1.0f;
    const float AngularOverlap = 0.15f * Pi / 180.0f;
    ImVec2 Centers[4];
    float Starts[4];
    PanelCorners(Minimum, Maximum, Radius, Centers, Starts);
    for (int CornerIndex = 0; CornerIndex < 4; ++CornerIndex) {
        for (int SegmentIndex = 0; SegmentIndex < CornerSegments; ++SegmentIndex) {
            const float StartAngle = CornerAngle(Starts[CornerIndex], SegmentIndex) - AngularOverlap;
            const float EndAngle = CornerAngle(Starts[CornerIndex], SegmentIndex + 1) + AngularOverlap;
            const float StartCosine = cosf(StartAngle), StartSine = sinf(StartAngle);
            const float EndCosine = cosf(EndAngle), EndSine = sinf(EndAngle);
            const ImVec2 Segment[4] = {
                ImVec2(Centers[CornerIndex].x + StartCosine * InnerRadius,
                       Centers[CornerIndex].y + StartSine * InnerRadius),
                ImVec2(Centers[CornerIndex].x + StartCosine * Radius,
                       Centers[CornerIndex].y + StartSine * Radius),
                ImVec2(Centers[CornerIndex].x + EndCosine * Radius,
                       Centers[CornerIndex].y + EndSine * Radius),
                ImVec2(Centers[CornerIndex].x + EndCosine * InnerRadius,
                       Centers[CornerIndex].y + EndSine * InnerRadius),
            };
            DrawList->AddConvexPolyFilled(Segment, 4, Color);
        }
    }
    DrawList->Flags = SavedFlags;
}

inline void DrawPanel(ImDrawList* DrawList, ImVec2 Minimum, ImVec2 Maximum, ImU32 Fill, float Radius) {
    FillPanel(DrawList, Minimum, Maximum, Fill, Radius);
    DrawPanelBorder(DrawList, Minimum, Maximum, Radius);
}

inline bool ContainsIgnoreCase(const std::string& Text, const std::string& Needle) {
    if (Needle.empty())
        return true;
    std::string LowerText = Text, LowerNeedle = Needle;
    for (auto& Character : LowerText)
        Character = (char)tolower((unsigned char)Character);
    for (auto& Character : LowerNeedle)
        Character = (char)tolower((unsigned char)Character);
    return LowerText.find(LowerNeedle) != std::string::npos;
}

inline void GetAccentHsv(const Vape::Accent& Accent, float Time, float& Hue, float& Saturation,
                         float& Value) {
    if (Accent.Rainbow) {
        Hue = fmodf(Time * 0.1f * Accent.RainbowSpeed, 1.0f);
        if (Hue < 0)
            Hue += 1.0f;
        Vape::GetRainbowSaturation(Hue, Saturation, Value);
    } else {
        Hue = Accent.Hue;
        Saturation = Accent.Saturation;
        Value = Accent.Value;
    }
}

inline float RowWidth() {
    float Width = ImGui::GetContentRegionAvail().x;
    if (Width >= 10.0f) {
        return Width;
    }
    ImGui::SetCursorPosX(0.0f);
    return ImGui::GetWindowWidth();
}

inline std::string TruncateToWidth(const std::string& Text, float MaxWidth) {
    if (MaxWidth <= 4.0f || ImGui::CalcTextSize(Text.c_str()).x <= MaxWidth)
        return Text;
    std::string Result = Text;
    while (!Result.empty() && ImGui::CalcTextSize((Result + "...").c_str()).x > MaxWidth)
        Result.pop_back();
    return Result.empty() ? std::string() : Result + "...";
}

inline ImVec4 GetAccentColor(const Vape::Accent& Accent, float Time) {
    float Hue, Saturation, Value;
    GetAccentHsv(Accent, Time, Hue, Saturation, Value);
    return Vape::ColorkeySafe(Vape::HsvToRgb(Hue, Saturation, Value));
}

// Every BeginPanel call requires EndPanel, even when it returns false.
inline bool BeginPanel(const char* Id, ImVec2 Position, ImVec2 Size) {
    ImGui::SetNextWindowPos(Position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(Size, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    return ImGui::Begin(Id, nullptr, Flags);
}

inline void EndPanel() {
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
