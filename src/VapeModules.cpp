#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::RenderModuleRow(Vape::Module& Module, float Time) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    float Width = RowWidth();

    ImVec4 ModuleAccent = GetAccentColor(Accent, Time);
    float AccentHue, AccentSaturation, AccentValue;
    GetAccentHsv(Accent, Time, AccentHue, AccentSaturation, AccentValue);

    std::string WidgetId = "##mod_" + Module.Name + "_" + std::to_string(Module.Index);
    ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(Width, ModuleHeight));
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    float DotsX0 = RowMax.x - 25.0f;
    bool InDots = ImGui::GetIO().MousePos.x >= DotsX0 && Hovered;

    bool Capturing = (BindingModule == &Module);
    std::string BindName = Module.BindKey ? GetKeyName(Module.BindKey) : "";
    float ChipWidth = 20.0f;
    if (!BindName.empty()) {
        ImVec2 BindTextSize = ImGui::CalcTextSize(BindName.c_str());
        ChipWidth = std::max(BindTextSize.x + 10.0f, 20.0f);
    }
    bool ShowChip = !EditMode && (Module.BindKey != 0 || Hovered || Module.Expanded || Capturing);
    ImVec2 ChipMin(0, 0), ChipMax(0, 0);
    bool ChipHover = false;
    if (ShowChip) {
        ChipMax = ImVec2(RowMax.x - 28.0f, RowMin.y + 30.0f);
        ChipMin = ImVec2(ChipMax.x - ChipWidth, RowMin.y + 10.0f);
        ImVec2 MousePosition = ImGui::GetIO().MousePos;
        ChipHover = Hovered && MousePosition.x >= ChipMin.x && MousePosition.x <= ChipMax.x &&
                    MousePosition.y >= ChipMin.y && MousePosition.y <= ChipMax.y;
    }

    if (!EditMode) {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (ChipHover) {
                bool Shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                if (Capturing) {
                    Module.BindKey = 0;
                    BindingModule = nullptr;
                } else if (Shift && Module.BindKey != 0) {
                    Module.HoldToEnable = !Module.HoldToEnable;
                } else {
                    BindingModule = &Module;
                    for (int I = 0; I < 52; ++I) {
                        int Vk = GetBindableKey(I);
                        if (Vk > 0 && Vk < 256)
                            KeysDownAtCapture[Vk] = (GetAsyncKeyState(Vk) & 0x8000) != 0;
                    }
                    KeysDownAtCapture[VK_ESCAPE] = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
                }
            } else if (Capturing) {
            } else if (InDots) {
                Module.Expanded = !Module.Expanded;
            } else {
                Module.Enabled = !Module.Enabled;
                if (ToggleNotifications)
                    NotifyModuleState(Module);
            }
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !Capturing)
            Module.Expanded = !Module.Expanded;
    } else {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            Module.Visible = !Module.Visible;
    }

    ImVec4 RowBackground;
    if (!EditMode && Module.Enabled)
        RowBackground = ModuleAccent;
    else if (Hovered || Module.Expanded)
        RowBackground = Lighten(MainColor(), 0.02f);
    else
        RowBackground = MainColor();

    DrawList->AddRectFilled(RowMin, RowMax, ToU32(RowBackground));

    ImVec4 NameColor;
    if (!EditMode && Module.Enabled)
        NameColor = GetAccentTextColor(AccentHue, AccentSaturation, AccentValue);
    else if (Hovered || Module.Expanded)
        NameColor = Text;
    else
        NameColor = DimText;
    if (EditMode && !Module.Visible)
        NameColor = Darken(Text, 0.35f);

    DrawList->AddText(ImVec2(RowMin.x + 14, RowMin.y + 11), ToU32(NameColor), Module.Name.c_str());

    if (!EditMode) {
        if (Capturing) {
            DrawList->AddRectFilled(RowMin, RowMax, IM_COL32(8, 8, 8, 220));
            const char* CaptureText = "PRESS A KEY TO BIND";
            ImVec2 CaptureTextSize = ImGui::CalcTextSize(CaptureText);
            DrawList->AddText(ImVec2((RowMin.x + RowMax.x - CaptureTextSize.x) * 0.5f,
                                     (RowMin.y + RowMax.y - CaptureTextSize.y) * 0.5f),
                              ToU32(Text), CaptureText);
        }

        if (ShowChip) {
            DrawList->AddRectFilled(ChipMin, ChipMax, IM_COL32(255, 255, 255, 20), 4.0f);
            ImVec4 ChipColor = ChipHover ? Darken(Text, 0.16f) : Darken(Text, 0.43f);
            ImVec2 ChipCenter((ChipMin.x + ChipMax.x) * 0.5f, (ChipMin.y + ChipMax.y) * 0.5f);
            if (Capturing) {
                const char* Dots = "...";
                ImVec2 DotsTextSize = ImGui::CalcTextSize(Dots);
                DrawList->AddText(
                    ImVec2(ChipCenter.x - DotsTextSize.x * 0.5f, ChipCenter.y - DotsTextSize.y * 0.5f),
                    ToU32(ChipColor), Dots);
            } else if (BindName.empty()) {
                DrawIcon("bind.png", ImVec2(ChipCenter.x - 5, ChipCenter.y - 5), ImVec2(10, 10),
                         ToU32(ChipColor));
            } else {
                ImVec2 BindTextSize = ImGui::CalcTextSize(BindName.c_str());
                DrawList->AddText(
                    ImVec2(ChipCenter.x - BindTextSize.x * 0.5f, ChipCenter.y - BindTextSize.y * 0.5f),
                    ToU32(ChipColor), BindName.c_str());
            }
            if (ChipHover) {
                std::string Tooltip =
                    "Click to bind\nShift click to modify bind functionality\nBind functionality = ";
                Tooltip += Module.HoldToEnable ? "Enable while held" : "Toggle";
                ShowTooltip(Tooltip.c_str());
            }
        }

        ImU32 DotsColor;
        if (Module.Enabled)
            DotsColor = ToU32(NameColor);
        else if (Hovered || InDots || Module.Expanded)
            DotsColor = ToU32(Text);
        else
            DotsColor = ToU32(Lighten(MainColor(), 0.37f));
        auto Icon = Icons.find("settingdots.png");
        if (Icon != Icons.end() && Icon->second.ShaderResource) {
            DrawList->AddImage((ImTextureID)Icon->second.ShaderResource, ImVec2(RowMax.x - 20, RowMin.y + 12),
                               ImVec2(RowMax.x - 17, RowMin.y + 28), ImVec2(0, 0), ImVec2(1, 1), DotsColor);
        } else {
            for (int I = 0; I < 3; ++I)
                DrawList->AddCircleFilled(ImVec2(RowMax.x - 18.5f, RowMin.y + 14 + I * 6), 1.5f, DotsColor);
        }

        if (Module.Enabled)
            DrawList->AddRectFilled(ImVec2(RowMin.x, RowMax.y - 1), RowMax, IM_COL32(48, 48, 48, 122));
        if (!ChipHover && !Module.Name.empty() && Hovered && !Module.Tooltip.empty())
            ShowTooltip(Module.Tooltip.c_str());
    } else {
        ImVec2 B0 = ImVec2(RowMax.x - 24, RowMin.y + 16);
        ImVec2 B1 = ImVec2(RowMax.x - 16, RowMin.y + 24);
        if (Module.Visible)
            DrawList->AddRectFilled(B0, B1, ToU32(ModuleAccent));
        else
            DrawList->AddRect(B0, B1, ToU32(Lighten(MainColor(), 0.37f)));
    }

    if (Module.Expanded && !EditMode) {
        RenderModuleOptions(Module, ModuleAccent);
    }
}

void VapeGui::RenderToggleOption(Vape::Module& Module, Vape::ToggleOption& Option,
                                 const ImVec4& AccentColor) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    float Width = RowWidth();

    std::string WidgetId = "##tgl_" + Option.Name + "_" + Module.Name;
    ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(Width, ToggleHeight));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        Option.Enabled = !Option.Enabled;
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Darken(MainColor(), 0.02f)));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 7), ToU32(Hovered || Option.Enabled ? Text : DimText),
                      Option.Name.c_str());

    ImVec2 SwitchMin = ImVec2(RowMax.x - 30, RowMin.y + 9);
    ImVec2 SwitchMax = ImVec2(RowMax.x - 8, RowMin.y + 21);
    ImVec4 HolderCol =
        Option.Enabled ? AccentColor : (Hovered ? Lighten(MainColor(), 0.37f) : Lighten(MainColor(), 0.14f));
    DrawList->AddRectFilled(SwitchMin, SwitchMax, ToU32(HolderCol), 6.0f);
    float KnobX = Option.Enabled ? (SwitchMax.x - 10) : (SwitchMin.x + 2);
    DrawList->AddRectFilled(ImVec2(KnobX, SwitchMin.y + 2), ImVec2(KnobX + 8, SwitchMin.y + 10),
                            ToU32(MainColor()), 4.0f);
}

void VapeGui::RenderSliderOption(Vape::SliderOption& Option, const ImVec4& AccentColor) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    float Width = RowWidth();

    std::string WidgetId = "##sld_" + Option.Name;
    ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(Width, SliderHeight));
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    bool Hovered = ImGui::IsItemHovered();
    bool Active = ImGui::IsItemActive();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Darken(MainColor(), 0.02f)));

    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 4), ToU32(DimText), Option.Name.c_str());
    char ValueBuffer[48];
    if (Option.RoundingScale >= 10.0f)
        snprintf(ValueBuffer, sizeof(ValueBuffer), "%g %s", Option.Value, Option.Suffix.c_str());
    else
        snprintf(ValueBuffer, sizeof(ValueBuffer), "%d %s", (int)Option.Value, Option.Suffix.c_str());
    ImVec2 ValueSize = ImGui::CalcTextSize(ValueBuffer);
    DrawList->AddText(ImVec2(RowMax.x - 10 - ValueSize.x, RowMin.y + 4), ToU32(DimText), ValueBuffer);

    ImVec2 TrackMin = ImVec2(RowMin.x + 10, RowMin.y + 37);
    ImVec2 TrackMax = ImVec2(RowMax.x - 10, RowMin.y + 39);
    DrawList->AddRectFilled(TrackMin, TrackMax, ToU32(Lighten(MainColor(), 0.034f)));
    float Range = Option.Maximum - Option.Minimum;
    float Fraction = Range > 0 ? (Option.Value - Option.Minimum) / Range : 0;
    Fraction = std::clamp(Fraction, 0.04f, 0.96f);
    float FillX = TrackMin.x + (TrackMax.x - TrackMin.x) * Fraction;
    DrawList->AddRectFilled(TrackMin, ImVec2(FillX, TrackMax.y), ToU32(AccentColor));

    float KnobRadius = (Hovered || Active) ? 8.0f : 7.0f;
    float KnobY = (TrackMin.y + TrackMax.y) * 0.5f;

    DrawList->AddCircleFilled(ImVec2(FillX, KnobY), KnobRadius + 5, ToU32(Darken(MainColor(), 0.02f)));
    DrawList->AddCircleFilled(ImVec2(FillX, KnobY), KnobRadius, ToU32(AccentColor));

    ImVec2 MousePosition = ImGui::GetIO().MousePos;
    if (Active && MousePosition.y > RowMin.y + 20) {
        float MouseFraction =
            std::clamp((MousePosition.x - TrackMin.x) / (TrackMax.x - TrackMin.x), 0.0f, 1.0f);
        float Raw = Option.Minimum + Range * MouseFraction;
        float NewValue = floorf(Raw * Option.RoundingScale) / Option.RoundingScale;
        NewValue = std::clamp(NewValue, Option.Minimum, Option.Maximum);
        if (NewValue != Option.Value)
            Option.Value = NewValue;
    }
}

void VapeGui::RenderTwoSliderOption(Vape::TwoSliderOption& Option, const ImVec4& AccentColor) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    float Width = RowWidth();

    ImVec2 RowMin = ImGui::GetCursorScreenPos();
    ImVec2 RowMax = ImVec2(RowMin.x + Width, RowMin.y + SliderHeight);
    ImVec2 MousePosition = ImGui::GetIO().MousePos;

    std::string WidgetId = "##tsl_" + Option.Name;
    ImGui::SetCursorScreenPos(RowMin);
    ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(Width, SliderHeight));
    bool Hovered = ImGui::IsItemHovered();
    bool Active = ImGui::IsItemActive();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Darken(MainColor(), 0.02f)));
    constexpr float FontSize = 11.0f;
    ImFont* Font = ImGui::GetFont();
    DrawList->AddText(Font, FontSize, ImVec2(RowMin.x + 10, RowMin.y + 11.5f), ToU32(DimText),
                      Option.Name.c_str());

    char LowerText[16], UpperText[16];
    snprintf(LowerText, sizeof(LowerText), "%g", Option.LowerValue);
    snprintf(UpperText, sizeof(UpperText), "%g", Option.UpperValue);
    float LowerWidth = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, LowerText).x;
    float UpperWidth = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, UpperText).x;
    constexpr float LabelGap = 6.0f;
    ImVec2 ArrowSize(10.0f, 5.0f);
    float UpperLabelX = RowMax.x - 10.0f - UpperWidth;
    ImVec2 ArrowPosition(UpperLabelX - LabelGap - ArrowSize.x,
                         RowMin.y + 11 + (FontSize - ArrowSize.y) * 0.5f);
    DrawList->AddText(Font, FontSize, ImVec2(ArrowPosition.x - LabelGap - LowerWidth, RowMin.y + 11),
                      ToU32(DimText), LowerText);
    DrawList->AddText(Font, FontSize, ImVec2(UpperLabelX, RowMin.y + 11), ToU32(DimText), UpperText);
    ImU32 ArrowColor = ToU32(Lighten(MainColor(), 0.14f));
    auto ArrowIcon = Icons.find("rangeindicator.png");
    if (ArrowIcon != Icons.end() && ArrowIcon->second.ShaderResource) {
        DrawList->AddImage((ImTextureID)ArrowIcon->second.ShaderResource, ArrowPosition,
                           ImVec2(ArrowPosition.x + ArrowSize.x, ArrowPosition.y + ArrowSize.y), ImVec2(0, 0),
                           ImVec2(1, 1), ArrowColor);
    } else {
        DrawList->AddRectFilled(ImVec2(ArrowPosition.x + 2.5f, ArrowPosition.y + 2),
                                ImVec2(ArrowPosition.x + 7.5f, ArrowPosition.y + 3), ArrowColor);
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y + 2.5f),
                                    ImVec2(ArrowPosition.x + 2.5f, ArrowPosition.y),
                                    ImVec2(ArrowPosition.x + 2.5f, ArrowPosition.y + 5), ArrowColor);
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x + 10, ArrowPosition.y + 2.5f),
                                    ImVec2(ArrowPosition.x + 7.5f, ArrowPosition.y + 5),
                                    ImVec2(ArrowPosition.x + 7.5f, ArrowPosition.y), ArrowColor);
    }

    ImVec2 TrackMin = ImVec2(RowMin.x + 10, RowMin.y + 37);
    ImVec2 TrackMax = ImVec2(RowMax.x - 10, RowMin.y + 39);
    DrawList->AddRectFilled(TrackMin, TrackMax, ToU32(Lighten(MainColor(), 0.034f)));
    float Range = Option.Maximum - Option.Minimum;
    auto GetFraction = [&](float Value) {
        float Fraction = Option.Maximum > 0.0f ? Value / Option.Maximum : 0.0f;
        return std::clamp(Fraction, 0.04f, 0.96f);
    };
    float LowerFraction = GetFraction(Option.LowerValue), UpperFraction = GetFraction(Option.UpperValue);
    auto AnimateFraction = [&](const char* Suffix, float Target) {
        ImGui::PushID(WidgetId.c_str());
        ImGui::PushID(Suffix);
        ImGuiStorage* Storage = ImGui::GetStateStorage();
        ImGuiID ValueId = ImGui::GetID("Value");
        float Value = Storage->GetFloat(ValueId, Target);
        float Blend = 1.0f - expf(-40.0f * ImGui::GetIO().DeltaTime);
        Value += (Target - Value) * Blend;
        if (std::fabs(Target - Value) < 0.0001f)
            Value = Target;
        else
            AnimationPending = true;
        Storage->SetFloat(ValueId, Value);
        ImGui::PopID();
        ImGui::PopID();
        return Value;
    };
    LowerFraction = AnimateFraction("Lower", LowerFraction);
    UpperFraction = AnimateFraction("Upper", UpperFraction);
    float LowerX = TrackMin.x + (TrackMax.x - TrackMin.x) * LowerFraction;
    float UpperX = TrackMin.x + (TrackMax.x - TrackMin.x) * std::max(LowerFraction, UpperFraction);
    DrawList->AddRectFilled(ImVec2(LowerX, TrackMin.y), ImVec2(UpperX, TrackMax.y), ToU32(AccentColor));

    float KnobY = (TrackMin.y + TrackMax.y) * 0.5f;
    auto DrawGrip = [&](float GripX, bool Upper) {
        bool GripHovered = Hovered && std::fabs(MousePosition.x - GripX) <= 8.0f &&
                           std::fabs(MousePosition.y - KnobY) <= 2.0f;
        ImGuiID HoverId = ImGui::GetID((WidgetId + (Upper ? "_upper" : "_lower")).c_str());
        ImGuiStorage* Storage = ImGui::GetStateStorage();
        float HoverAmount = Storage->GetFloat(HoverId);
        float Target = GripHovered ? 1.0f : 0.0f;
        HoverAmount += (Target - HoverAmount) * (1.0f - expf(-20.0f * ImGui::GetIO().DeltaTime));
        if (std::fabs(Target - HoverAmount) < 0.001f)
            HoverAmount = Target;
        else
            AnimationPending = true;
        Storage->SetFloat(HoverId, HoverAmount);
        DrawList->AddRectFilled(ImVec2(GripX - 8, KnobY - 2), ImVec2(GripX + 8, KnobY + 2),
                                ToU32(Darken(MainColor(), 0.02f)));
        float HalfWidth = 4.5f + HoverAmount, HalfHeight = 8.0f + HoverAmount;
        auto Icon = Icons.find("range.png");
        if (Icon != Icons.end() && Icon->second.ShaderResource) {
            DrawList->AddImage(
                (ImTextureID)Icon->second.ShaderResource, ImVec2(GripX - HalfWidth, KnobY - HalfHeight),
                ImVec2(GripX + HalfWidth, KnobY + HalfHeight), Upper ? ImVec2(1, 1) : ImVec2(0, 0),
                Upper ? ImVec2(0, 0) : ImVec2(1, 1), ToU32(AccentColor));
        } else {
            float Direction = Upper ? -1.0f : 1.0f;
            DrawList->AddTriangleFilled(ImVec2(GripX + Direction * HalfWidth, KnobY),
                                        ImVec2(GripX - Direction * HalfWidth, KnobY + Direction * HalfHeight),
                                        ImVec2(GripX - Direction * HalfWidth, KnobY - Direction * HalfHeight),
                                        ToU32(AccentColor));
        }
    };
    DrawGrip(LowerX, false);
    DrawGrip(UpperX, true);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        DraggedRangeHandle =
            MousePosition.y > RowMin.y + 20 ? ((MousePosition.x - UpperX) > -18.0f ? 2 : 1) : 0;
    if (Active && DraggedRangeHandle != 0 && MousePosition.y > RowMin.y + 20) {
        float MouseFraction =
            std::clamp((MousePosition.x - TrackMin.x) / (TrackMax.x - TrackMin.x), 0.0f, 1.0f);
        float NewValue = std::clamp(floorf((Option.Minimum + Range * MouseFraction) * Option.RoundingScale) /
                                        Option.RoundingScale,
                                    Option.Minimum, Option.Maximum);
        if (DraggedRangeHandle == 2)
            Option.UpperValue = NewValue;
        else
            Option.LowerValue = NewValue;
    }
}

void VapeGui::RenderDropdownOption(Vape::DropdownOption& Option) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    float Width = RowWidth();

    std::string WidgetId = "##dd_" + Option.Name;
    ImGui::InvisibleButton(WidgetId.c_str(), ImVec2(Width, 30));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        Option.Open = !Option.Open;
    bool Hovered = ImGui::IsItemHovered();
    ImVec2 RowMin = ImGui::GetItemRectMin(), RowMax = ImGui::GetItemRectMax();
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Darken(MainColor(), 0.02f)));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 7), ToU32(Hovered ? Text : DimText),
                      Option.Name.c_str());
    const char* Current = Option.Selected >= 0 && Option.Selected < (int)Option.Items.size()
                              ? Option.Items[Option.Selected].c_str()
                              : "";
    ImVec2 CurrentSize = ImGui::CalcTextSize(Current);
    DrawList->AddText(ImVec2(RowMax.x - 26 - CurrentSize.x, RowMin.y + 7), ToU32(DimText), Current);

    ImVec2 ArrowPosition = ImVec2(RowMax.x - 18, RowMin.y + 13);
    ImU32 ArrowColor = ToU32(Lighten(MainColor(), 0.37f));
    if (Option.Open)
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y),
                                    ImVec2(ArrowPosition.x + 9, ArrowPosition.y),
                                    ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y + 4.5f), ArrowColor);
    else
        DrawList->AddTriangleFilled(ImVec2(ArrowPosition.x, ArrowPosition.y + 4.5f),
                                    ImVec2(ArrowPosition.x + 9, ArrowPosition.y + 4.5f),
                                    ImVec2(ArrowPosition.x + 4.5f, ArrowPosition.y), ArrowColor);

    if (Option.Open) {
        for (int I = 0; I < (int)Option.Items.size(); ++I) {
            std::string ItemId = "##ddi_" + Option.Name + "_" + std::to_string(I);
            ImGui::InvisibleButton(ItemId.c_str(), ImVec2(Width, 26));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                Option.Selected = I;
                Option.Open = false;
            }
            bool ItemHovered = ImGui::IsItemHovered();
            ImVec2 I0 = ImGui::GetItemRectMin(), I1 = ImGui::GetItemRectMax();
            if (ItemHovered)
                DrawList->AddRectFilled(I0, I1, ToU32(Lighten(MainColor(), 0.06f)));
            DrawList->AddText(ImVec2(I0.x + 22, I0.y + 5), ToU32(I == Option.Selected ? Text : DimText),
                              Option.Items[I].c_str());
        }
    }
}

void VapeGui::RenderTextBoxOption(Vape::TextBoxOption& Option) {
    using namespace Vape;
    ImVec4 Text = TextColor();
    ImVec4 DimText = Darken(Text, 0.16f);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    float Width = RowWidth();

    ImVec2 RowMin = ImGui::GetCursorScreenPos();

    // A button here would steal hover from the text field.
    ImGui::Dummy(ImVec2(Width, 40));
    ImVec2 RowMax = ImVec2(RowMin.x + Width, RowMin.y + 40);
    DrawList->AddRectFilled(RowMin, RowMax, ToU32(Darken(MainColor(), 0.02f)));
    DrawList->AddText(ImVec2(RowMin.x + 10, RowMin.y + 12), ToU32(DimText), Option.Name.c_str());

    ImGui::SetCursorScreenPos(ImVec2(RowMax.x - 122, RowMin.y + 9));
    ImGui::PushItemWidth(112);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ToU32(Lighten(MainColor(), 0.06f)));
    ImGui::PushStyleColor(ImGuiCol_Text, ToU32(Text));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputText(("##tbin_" + Option.Name).c_str(), Option.Text, IM_ARRAYSIZE(Option.Text));
    ImGui::PopItemWidth();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void VapeGui::RenderModuleOptions(Vape::Module& Module, const ImVec4& AccentColor) {
    for (auto& Option : Module.Options) {
        switch (Option.Type) {
        case Vape::OptionType::Toggle:
            RenderToggleOption(Module, Option.Toggle, AccentColor);
            break;
        case Vape::OptionType::Slider:
            RenderSliderOption(Option.Slider, AccentColor);
            break;
        case Vape::OptionType::TwoSlider:
            RenderTwoSliderOption(Option.TwoSlider, AccentColor);
            break;
        case Vape::OptionType::Dropdown:
            RenderDropdownOption(Option.Dropdown);
            break;
        case Vape::OptionType::TextBox:
            RenderTextBoxOption(Option.TextBox);
            break;
        }
    }
}
