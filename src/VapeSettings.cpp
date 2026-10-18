#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

bool VapeGui::RenderToggleRow(const char* Label, bool& Value, const char* WidgetId,
                              const ImVec4& AccentColor) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float Width = RowWidth();
    ImGui::InvisibleButton(WidgetId, ImVec2(Width, 30));
    bool Changed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    if (Changed)
        Value = !Value;
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    DrawList->AddRectFilled(RowMin, RowMax, ToU32(MainColor()));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 7), ToU32(Hovered || Value ? Text : DimText), Label);
    ImVec2 SwitchMin = ImVec2(RowMax.x - 30, RowMin.y + 9);
    ImVec2 SwitchMax = ImVec2(RowMax.x - 8, RowMin.y + 21);
    ImVec4 SwitchColor =
        Value ? AccentColor : (Hovered ? Lighten(MainColor(), 0.37f) : Lighten(MainColor(), 0.14f));
    DrawList->AddRectFilled(SwitchMin, SwitchMax, ToU32(SwitchColor), 6.0f);
    float KnobX = Value ? (SwitchMax.x - 10) : (SwitchMin.x + 2);
    DrawList->AddRectFilled(ImVec2(KnobX, SwitchMin.y + 2), ImVec2(KnobX + 8, SwitchMin.y + 10),
                            ToU32(MainColor()), 4.0f);
    return Changed;
}

bool VapeGui::RenderNavigationButton(const char* Label, const char* WidgetId) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float Width = RowWidth();
    ImGui::InvisibleButton(WidgetId, ImVec2(Width, 40));
    bool Clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Hovered ? Lighten(MainColor(), 0.02f) : MainColor()));
    DrawList->AddText(ImVec2(RowMin.x + 14, RowMin.y + 12), ToU32(Hovered ? Text : DimText), Label);
    DrawIcon("expandarrow.png", ImVec2(RowMax.x - 18, RowMin.y + 16), ImVec2(5, 9),
             ToU32(Lighten(MainColor(), 0.37f)));
    return Clicked;
}

bool VapeGui::RenderActionButton(const char* Label, const char* WidgetId) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float Width = RowWidth();
    ImGui::InvisibleButton(WidgetId, ImVec2(Width, 34));
    bool Hovered = ImGui::IsItemHovered();
    bool Clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Hovered ? Lighten(MainColor(), 0.02f) : MainColor()));
    ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    DrawList->AddText(ImVec2(RowMin.x + (Width - LabelSize.x) * 0.5f, RowMin.y + 9),
                      ToU32(Hovered ? Text : DimText), Label);
    return Clicked;
}

void VapeGui::RenderAccentSliders() {
    using namespace Vape;
    ImVec4 DimText = Darken(TextColor(), 0.16f);
    ImVec4 AccentColor = GetAccentColor(Accent, (float)ImGui::GetTime());
    struct HSVSlider {
        const char* Name;
        float* Value;
    };
    HSVSlider Sliders[] = {
        {"Hue", &Accent.Hue}, {"Saturation", &Accent.Saturation}, {"Value", &Accent.Value}};
    for (auto& Slider : Sliders) {
        float Width = RowWidth();
        std::string SliderId = std::string("##hsv_") + Slider.Name;
        ImGui::InvisibleButton(SliderId.c_str(), ImVec2(Width, 50));
        ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
        bool Active = ImGui::IsItemActive();
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(RowMin, RowMax, ToU32(MainColor()));
        DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 4), ToU32(DimText), Slider.Name);
        char ValueBuffer[16];
        snprintf(ValueBuffer, sizeof(ValueBuffer), "%d", (int)(*Slider.Value * 100));
        ImVec2 ValueSize = ImGui::CalcTextSize(ValueBuffer);
        DrawList->AddText(ImVec2(RowMax.x - 10 - ValueSize.x, RowMin.y + 4), ToU32(DimText), ValueBuffer);
        ImVec2 TrackMin = ImVec2(RowMin.x + 10, RowMin.y + 37);
        ImVec2 TrackMax = ImVec2(RowMax.x - 10, RowMin.y + 39);

        if (std::string(Slider.Name) == "Hue") {
            const int Segs = 24;
            for (int I = 0; I < Segs; ++I) {
                float F0 = (float)I / Segs, F1 = (float)(I + 1) / Segs;
                DrawList->AddRectFilled(ImVec2(TrackMin.x + (TrackMax.x - TrackMin.x) * F0, TrackMin.y),
                                        ImVec2(TrackMin.x + (TrackMax.x - TrackMin.x) * F1, TrackMax.y),
                                        ToU32(HsvToRgb(F0, 1, 1)));
            }
        } else {
            DrawList->AddRectFilled(TrackMin, TrackMax, ToU32(Lighten(MainColor(), 0.034f)));
        }
        float Fraction = std::clamp(*Slider.Value, 0.04f, 0.96f);
        float FillX = TrackMin.x + (TrackMax.x - TrackMin.x) * Fraction;
        if (std::string(Slider.Name) != "Hue")
            DrawList->AddRectFilled(TrackMin, ImVec2(FillX, TrackMax.y), ToU32(AccentColor));
        else
            DrawList->AddRectFilled(ImVec2(FillX - 1, TrackMin.y - 3), ImVec2(FillX + 1, TrackMax.y + 3),
                                    IM_COL32(255, 255, 255, 255));
        DrawList->AddCircleFilled(ImVec2(FillX, (TrackMin.y + TrackMax.y) * 0.5f), 7.0f, ToU32(AccentColor));
        DrawList->AddCircle(ImVec2(FillX, (TrackMin.y + TrackMax.y) * 0.5f), 7.0f,
                            IM_COL32(255, 255, 255, 90));
        ImVec2 MousePosition = ImGui::GetIO().MousePos;
        if (Active && MousePosition.y > RowMin.y + 20) {
            *Slider.Value =
                std::clamp((MousePosition.x - TrackMin.x) / (TrackMax.x - TrackMin.x), 0.0f, 1.0f);
        }
    }
}

void VapeGui::RenderPresetSwatches() {
    using namespace Vape;
    ImVec4 DimText = Darken(TextColor(), 0.16f);
    ImU32 Presets[] = {
        IM_COL32(250, 50, 56, 255),  IM_COL32(242, 99, 33, 255),  IM_COL32(252, 179, 22, 255),
        IM_COL32(5, 133, 104, 255),  IM_COL32(47, 122, 229, 255), IM_COL32(126, 84, 217, 255),
        IM_COL32(232, 96, 152, 255),
    };

    float Width = RowWidth();
    ImGui::InvisibleButton("##swatches", ImVec2(Width, 40));
    ImVec2 RowMin = ImGui::GetItemRectMin();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RowMin, ImVec2(RowMin.x + Width, RowMin.y + 40), ToU32(MainColor()));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 2), ToU32(DimText), "Presets");
    float SwatchX = RowMin.x + 10;
    for (int I = 0; I < 7; ++I) {
        ImVec2 SwatchMin = ImVec2(SwatchX, RowMin.y + 18);
        ImVec2 SwatchMax = ImVec2(SwatchX + 20, RowMin.y + 34);
        DrawList->AddRectFilled(SwatchMin, SwatchMax, Presets[I], 3.0f);
        ImVec2 MousePosition = ImGui::GetIO().MousePos;
        bool Inside = MousePosition.x >= SwatchMin.x && MousePosition.x <= SwatchMax.x &&
                      MousePosition.y >= SwatchMin.y && MousePosition.y <= SwatchMax.y;
        if (Inside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            float Red = ((Presets[I] >> 0) & 255) / 255.0f;
            float Green = ((Presets[I] >> 8) & 255) / 255.0f;
            float Blue = ((Presets[I] >> 16) & 255) / 255.0f;
            float Hue, Saturation, Value;
            ImGui::ColorConvertRGBtoHSV(Red, Green, Blue, Hue, Saturation, Value);
            Accent.Hue = Hue;
            Accent.Saturation = Saturation;
            Accent.Value = Value;
        }
        SwatchX += 26;
    }
}

void VapeGui::RenderDisplaySelector() {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float Width = RowWidth();
    ImGui::InvisibleButton("##display_dd", ImVec2(Width, 34));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        MonitorDropdownOpen = !MonitorDropdownOpen;
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(MainColor()));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 9), ToU32(Hovered ? Text : DimText), "Display");
    std::string Current = (MonitorIndex >= 0 && MonitorIndex < (int)MonitorNames.size())
                              ? MonitorNames[MonitorIndex]
                              : "Default";
    float MaxWidth = Width - 110.0f;
    std::string Shown = Current;
    while (Shown.size() > 4 && ImGui::CalcTextSize(Shown.c_str()).x > MaxWidth)
        Shown.pop_back();
    if (Shown.size() != Current.size())
        Shown += "...";
    ImVec2 SelectionSize = ImGui::CalcTextSize(Shown.c_str());
    DrawList->AddText(ImVec2(RowMax.x - 26 - SelectionSize.x, RowMin.y + 9), ToU32(DimText), Shown.c_str());
    ImVec2 ArrowPosition = ImVec2(RowMax.x - 18, RowMin.y + 15);
    ImU32 ArrowColor = ToU32(Lighten(MainColor(), 0.37f));
    if (MonitorDropdownOpen)
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y),
                                    ImVec2(ArrowPosition.x + 9, ArrowPosition.y),
                                    ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y + 4.5f), ArrowColor);
    else
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y + 4.5f),
                                    ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4.5f),
                                    ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y), ArrowColor);
    if (MonitorDropdownOpen) {
        for (int I = 0; I < (int)MonitorNames.size(); ++I) {
            std::string ItemId = "##moni_" + std::to_string(I);
            ImGui::InvisibleButton(ItemId.c_str(), ImVec2(Width, 26));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (I != MonitorIndex) {
                    MonitorIndex = I;
                    if (OnMonitorChange)
                        OnMonitorChange(I);
                }
                MonitorDropdownOpen = false;
            }
            bool ItemHovered = ImGui::IsItemHovered();
            ImVec2 I0 = ImGui::GetItemRectMin(), I1 = ImGui::GetItemRectMax();
            if (ItemHovered)
                DrawList->AddRectFilled(I0, I1, ToU32(Lighten(MainColor(), 0.06f)));
            DrawList->AddText(ImVec2(I0.x + 22, I0.y + 5), ToU32(I == MonitorIndex ? Text : DimText),
                              MonitorNames[I].c_str());
        }
    }
}

ImVec2 VapeGui::GetSettingsSize() const {
    using namespace Vape;
    float ListHeight = (SettingsPage == Vape::SettingsView::Debug && MonitorDropdownOpen)
                           ? (float)MonitorNames.size() * 26.0f
                           : 0.0f;

    float Rows = 0.0f;
    if (SettingsPage == Vape::SettingsView::Visual) {
        Rows = 30 + 3 * 50 + 40;
    } else if (SettingsPage == Vape::SettingsView::Debug) {
        Rows = 34 + ListHeight + 30 + 30 + 30;
    } else {
        Rows = 30 + 30 + 30 + 40 + 40 + 34 + 34;
    }

    float Height = std::max(41.0f + Rows + BottomGap, GetMainSize().y);
    return ImVec2(PanelWidth, Height);
}

void VapeGui::RenderSettings() {
    float Time = (float)ImGui::GetTime();
    ImVec4 AccentColor = GetAccentColor(Accent, Time);

    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);

    float PanelX = MainX;
    float PanelY = MainY;
    float PanelHeight = GetSettingsSize().y;

    bool Open = BeginPanel("##VapeSettings", ImVec2(PanelX, PanelY), ImVec2(PanelWidth, PanelHeight));
    ImVec2 WindowPosition = ImGui::GetWindowPos();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Open) {
        DrawPanel(DrawList, WindowPosition,
                  ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + PanelHeight), ToU32(MainColor()),
                  PanelRadius);
        const char* PageTitle = SettingsPage == Vape::SettingsView::Visual
                                    ? "Visual"
                                    : (SettingsPage == Vape::SettingsView::Debug ? "Debug" : "Settings");

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##settings_back", ImVec2(30, RowsTop));
        if (ImGui::IsItemClicked()) {
            if (SettingsPage != Vape::SettingsView::Root)
                SettingsPage = Vape::SettingsView::Root;
            else {
                SettingsOpen = false;
                SettingsPage = Vape::SettingsView::Root;
            }
        }
        bool BackHover = ImGui::IsItemHovered();
        DrawIcon("backmini.png", ImVec2(WindowPosition.x + 12, WindowPosition.y + 14), ImVec2(14, 14),
                 ToU32(BackHover ? Text : Lighten(MainColor(), 0.37f)));
        DrawList->AddText(ImVec2(WindowPosition.x + 36, WindowPosition.y + 12), ToU32(Text), PageTitle);

        ImGui::SetCursorPos(ImVec2(PanelWidth - 28, 11));
        ImGui::InvisibleButton("##settings_close", ImVec2(20, 20));
        bool CloseHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            SettingsOpen = false;
            SettingsPage = Vape::SettingsView::Root;
        }
        DrawIcon("close.png", ImVec2(WindowPosition.x + PanelWidth - 24, WindowPosition.y + 15),
                 ImVec2(12, 12), ToU32(CloseHover ? Text : DimText));

        DrawList->AddRectFilled(ImVec2(WindowPosition.x, WindowPosition.y + 41),
                                ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + 42),
                                IM_COL32(255, 255, 255, 18));

        ImGui::SetCursorPos(ImVec2(0, 41));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        BeginScrollableChild("##settings_list", ImVec2(PanelWidth, PanelHeight - 49));
        ImDrawList* ContentDrawList = ImGui::GetWindowDrawList();

        if (SettingsPage == Vape::SettingsView::Visual) {
            RenderToggleRow("Rainbow", Accent.Rainbow, "##s_rainbow", AccentColor);
            RenderAccentSliders();
            RenderPresetSwatches();
        } else if (SettingsPage == Vape::SettingsView::Debug) {
            RenderDisplaySelector();
            if (RenderToggleRow("Always on top", AlwaysOnTop, "##d_topmost", AccentColor)) {
                if (OnTopmostChange)
                    OnTopmostChange(AlwaysOnTop);
            }
            if (RenderToggleRow("Fullscreen", Fullscreen, "##d_fullscreen", AccentColor)) {
                if (OnFullscreenChange)
                    OnFullscreenChange(Fullscreen);
            }
        } else {
            RenderToggleRow("Text GUI", TextGuiEnabled, "##s_textgui", AccentColor);
            RenderToggleRow("Toggle notifications", ToggleNotifications, "##s_notif", AccentColor);

            if (RenderToggleRow("Streamproof", Streamproof, "##s_Streamproof", AccentColor)) {
                if (OnStreamproofChange)
                    OnStreamproofChange(Streamproof);
            }
            if (RenderNavigationButton("Visual", "##s_visual"))
                SettingsPage = Vape::SettingsView::Visual;
            if (RenderNavigationButton("Debug", "##s_debug"))
                SettingsPage = Vape::SettingsView::Debug;

            if (RenderActionButton("Reset positions", "##resetpos")) {
                ResetPositions();
                Notify("Vape", "Panel positions reset", 2.0f);
            }

            if (RenderActionButton("Uninject", "##uninject")) {
                for (auto& Category : Categories)
                    for (auto& Module : Category.Modules)
                        Module.Enabled = false;
                PostQuitMessage(0);
            }
        }

        ContentDrawList->PushClipRect(
            WindowPosition, ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + PanelHeight), false);
        DrawPanelBorder(ContentDrawList, WindowPosition,
                        ImVec2(WindowPosition.x + PanelWidth, WindowPosition.y + PanelHeight), PanelRadius);
        ContentDrawList->PopClipRect();
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    EndPanel();

    HitRects.push_back(HitRect{PanelX, PanelY, PanelX + PanelWidth, PanelY + PanelHeight});
}
