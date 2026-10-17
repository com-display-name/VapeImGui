#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Vape {
inline ImVec4 MainColor() {
    return ImVec4(26.0f / 255.0f, 25.0f / 255.0f, 26.0f / 255.0f, 1.0f);
}
inline ImVec4 TextColor() {
    return ImVec4(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.0f);
}

inline constexpr float PanelWidth = 220.0f;
inline constexpr float HeaderHeight = 41.0f;
inline constexpr float RowsTop = 37.0f;
inline constexpr float ModuleHeight = 40.0f;
inline constexpr float ToggleHeight = 30.0f;
inline constexpr float SliderHeight = 50.0f;
inline constexpr float PanelRadius = 8.0f;

// Keep square rows above the rounded bottom corners.
inline constexpr float BottomGap = PanelRadius;
inline constexpr float MaxContentHeight = 560.0f;

inline constexpr float SearchHeight = 37.0f;
inline constexpr float SearchTop = 13.0f;

inline constexpr float PanelTop = SearchTop + SearchHeight + 16.0f;

inline constexpr float LegitWidth = 700.0f;
inline constexpr float LegitHeight = 380.0f;

struct Accent {
    float Hue = 0.46f;
    float Saturation = 0.96f;
    float Value = 0.52f;
    bool Rainbow = false;
    float RainbowSpeed = 1.0f;
};

// Pure black is transparent in the overlay. Keep every channel above zero.
inline ImVec4 ColorkeySafe(ImVec4 Color) {
    const float MinimumChannel = 1.5f / 255.0f;
    Color.x = std::max(Color.x, MinimumChannel);
    Color.y = std::max(Color.y, MinimumChannel);
    Color.z = std::max(Color.z, MinimumChannel);
    return Color;
}

inline void RgbToHsv(ImVec4 Color, float& Hue, float& Saturation, float& Value) {
    ImGui::ColorConvertRGBtoHSV(Color.x, Color.y, Color.z, Hue, Saturation, Value);
}
inline ImVec4 HsvToRgb(float Hue, float Saturation, float Value, float Alpha = 1.0f) {
    float Red, Green, Blue;
    ImGui::ColorConvertHSVtoRGB(Hue, Saturation, Value, Red, Green, Blue);
    return ImVec4(Red, Green, Blue, Alpha);
}
inline ImVec4 Darken(ImVec4 Color, float Amount) {
    float Hue, Saturation, Value;
    RgbToHsv(Color, Hue, Saturation, Value);

    Value = std::clamp(Value - Amount, 0.0f, 1.0f);
    return HsvToRgb(Hue, Saturation, Value, Color.w);
}
inline ImVec4 Lighten(ImVec4 Color, float Amount) {
    float Hue, Saturation, Value;
    RgbToHsv(Color, Hue, Saturation, Value);
    Value = std::clamp(Value + Amount, 0.0f, 1.0f);
    return HsvToRgb(Hue, Saturation, Value, Color.w);
}

inline void GetRainbowSaturation(float Hue, float& SaturationOut, float& ValueOut) {
    float Saturation = 0.74f + 0.26f * std::min(Hue / 0.045f, 1.0f);
    if (Hue > 0.577f)
        Saturation = 1.0f - 0.48f * std::min((Hue - 0.577f) / 0.088f, 1.0f);
    if (Hue > 0.674f)
        Saturation = 0.52f + 0.48f * std::min((Hue - 0.674f) / 0.149f, 1.0f);
    if (Hue > 0.869f)
        Saturation = 1.0f - 0.26f * std::min((Hue - 0.869f) / 0.131f, 1.0f);
    SaturationOut = Saturation;
    ValueOut = 1.0f;
}

inline ImVec4 GetAccentTextColor(float Hue, float Saturation, float Value) {
    if (Value >= 0.7f && (Saturation < 0.6f || (Hue > 0.04f && Hue < 0.56f)))
        return ImVec4(0.19f, 0.19f, 0.19f, 1.0f);
    return ImVec4(1, 1, 1, 1);
}

} // namespace Vape
