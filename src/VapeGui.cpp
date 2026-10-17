#include "VapeGui.h"
#include "VapeInternal.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

VapeGui Gui;

void VapeGui::SetIcon(const std::string& Name, ID3D11ShaderResourceView* ShaderResource, int Width,
                      int Height) {
    Icons[Name] = VapeIcon{ShaderResource, Width, Height};
}

void VapeGui::Notify(const std::string& Title, const std::string& Text, float Duration,
                     Vape::NotificationKind Kind) {
    if (!ToggleNotifications && Kind == Vape::NotificationKind::Info)
        return;
    Vape::Notification Notification;
    Notification.Title = Title;
    Notification.Text = Text;
    Notification.Created = ElapsedSeconds();
    Notification.Duration = Duration;
    Notification.Kind = Kind;
    Notifications.push_back(Notification);
    if (Notifications.size() > 5)
        Notifications.erase(Notifications.begin());
}

float VapeGui::GetOptionHeight(const Vape::Option& Option) const {
    switch (Option.Type) {
    case Vape::OptionType::Toggle:
        return Vape::ToggleHeight;
    case Vape::OptionType::Slider:
        return Vape::SliderHeight;
    case Vape::OptionType::TwoSlider:
        return Vape::SliderHeight;
    case Vape::OptionType::Dropdown:
        return 30.0f + (Option.Dropdown.Open ? (float)Option.Dropdown.Items.size() * 26.0f : 0.0f);
    case Vape::OptionType::TextBox:
        return 40.0f;
    }
    return 30.0f;
}

float VapeGui::GetModuleHeight(const Vape::Module& Module) const {
    float Height = ModuleHeight;
    if (Module.Expanded) {
        for (auto& Option : Module.Options)
            Height += GetOptionHeight(Option);
    }
    return Height;
}

void VapeGui::ShowTooltip(const char* Text, float Delay) {
    if (!Text || !Text[0])
        return;
    const int Frame = ImGui::GetFrameCount();
    const ImGuiID SourceId =
        GImGui->LastItemData.ID ? GImGui->LastItemData.ID : ImGui::GetCurrentWindow()->ID;
    const ImGuiID Id = ImHashStr(Text, 0, SourceId);
    if (TooltipId != Id || (TooltipFrame != Frame && TooltipFrame != Frame - 1)) {
        TooltipId = Id;
        TooltipStarted = ImGui::GetTime();
        TooltipPositionValid = false;
    }
    TooltipFrame = Frame;
    if (ImGui::GetTime() - TooltipStarted < std::max(Delay, 0.0f)) {
        AnimationPending = true;
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, Vape::MainColor());
    if (ImGui::BeginTooltip()) {
        const float Age = static_cast<float>(ImGui::GetTime() - TooltipStarted) - std::max(Delay, 0.0f);
        const float Progress = std::clamp(Age / 0.16f, 0.0f, 1.0f);
        const float Eased = 1.0f - std::pow(1.0f - Progress, 3.0f);
        const ImVec4 Background = Vape::MainColor();
        const ImVec4 Foreground = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(Background.x + (Foreground.x - Background.x) * Eased,
                                     Background.y + (Foreground.y - Background.y) * Eased,
                                     Background.z + (Foreground.z - Background.z) * Eased, Foreground.w));
        ImGui::TextUnformatted(Text);
        ImGui::PopStyleColor();

        ImGuiWindow* Window = ImGui::GetCurrentWindow();
        const ImVec2 Target = Window->Pos;
        if (!TooltipPositionValid) {
            TooltipPosition = Target;
            TooltipPositionValid = true;
        }
        const float Blend = 1.0f - std::exp(-24.0f * std::min(ImGui::GetIO().DeltaTime, 1.0f / 30.0f));
        TooltipPosition.x += (Target.x - TooltipPosition.x) * Blend;
        TooltipPosition.y += (Target.y - TooltipPosition.y) * Blend;
        if (std::fabs(Target.x - TooltipPosition.x) + std::fabs(Target.y - TooltipPosition.y) < 0.5f)
            TooltipPosition = Target;
        else
            AnimationPending = true;
        AnimationPending |= Progress < 1.0f;

        const ImRect Bounds = Window->Viewport->GetMainRect();
        const ImVec2 Position(std::clamp(TooltipPosition.x, Bounds.Min.x,
                                         std::max(Bounds.Min.x, Bounds.Max.x - Window->Size.x)),
                              std::clamp(TooltipPosition.y + 6.0f * (1.0f - Eased), Bounds.Min.y,
                                         std::max(Bounds.Min.y, Bounds.Max.y - Window->Size.y)));
        const ImVec2 Offset(Position.x - Target.x, Position.y - Target.y);
        // Move the opaque tooltip without fading against the overlay's black transparency key.
        for (auto& Vertex : Window->DrawList->VtxBuffer) {
            Vertex.pos.x += Offset.x;
            Vertex.pos.y += Offset.y;
        }
        for (auto& Command : Window->DrawList->CmdBuffer) {
            Command.ClipRect.x += Offset.x;
            Command.ClipRect.y += Offset.y;
            Command.ClipRect.z += Offset.x;
            Command.ClipRect.w += Offset.y;
        }
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

bool VapeGui::IsAnimating() const {
    return AnimationPending;
}

void VapeGui::BeginScrollableChild(const char* Id, ImVec2 Size) {
    auto& Animation = ScrollAnimations[ImGui::GetID(Id)];
    ImGuiWindow* Window = ImGui::FindWindowByID(Animation.WindowId);
    const int Frame = ImGui::GetFrameCount();
    bool ImmediateScroll = false;
    if (Window) {
        if (Animation.LastFrame != Frame - 1) {
            Animation.Position = Window->Scroll.y;
            Animation.Target = Window->Scroll.y;
        }
        const bool WheelInput = GImGui->WheelingWindow == Window &&
                                GImGui->WheelingWindowScrolledFrame == Frame &&
                                Window->ScrollTarget.y != FLT_MAX;
        if (WheelInput) {
            // Keep ImGui's wheel routing and accumulate input while the animation is moving.
            Animation.Target += Window->ScrollTarget.y - Window->Scroll.y;
        } else if (Window->ScrollTarget.y != FLT_MAX) {
            // Keyboard navigation and explicit scroll requests retain their normal behavior.
            ImmediateScroll = true;
        }
        Animation.Target = std::clamp(Animation.Target, 0.0f, Window->ScrollMax.y);
        Animation.Position = std::clamp(Animation.Position, 0.0f, Window->ScrollMax.y);
        if (WheelInput || Window->ScrollTarget.y == FLT_MAX) {
            const float DeltaTime = std::min(ImGui::GetIO().DeltaTime, 1.0f / 30.0f);
            const float Blend = 1.0f - std::exp(-18.0f * DeltaTime);
            Animation.Position += (Animation.Target - Animation.Position) * Blend;
            if (std::fabs(Animation.Target - Animation.Position) < 0.5f)
                Animation.Position = Animation.Target;
            else
                AnimationPending = true;
            ImGui::SetNextWindowScroll(ImVec2(-1.0f, Animation.Position));
        }
    }
    ImGui::BeginChild(Id, Size, false,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoScrollbar);
    Window = ImGui::GetCurrentWindow();
    Animation.WindowId = Window->ID;
    Animation.LastFrame = Frame;
    if (ImmediateScroll)
        Animation.Position = Animation.Target = Window->Scroll.y;
    Animation.Position = std::clamp(Animation.Position, 0.0f, Window->ScrollMax.y);
    Animation.Target = std::clamp(Animation.Target, 0.0f, Window->ScrollMax.y);
}

void VapeGui::DrawIcon(const char* Name, ImVec2 Position, ImVec2 Size, ImU32 Tint) {
    auto Icon = Icons.find(Name);
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    if (Icon != Icons.end() && Icon->second.ShaderResource) {
        DrawList->AddImage((ImTextureID)Icon->second.ShaderResource, Position,
                           ImVec2(Position.x + Size.x, Position.y + Size.y), ImVec2(0, 0), ImVec2(1, 1),
                           Tint);
    } else {
        DrawList->AddRectFilled(Position, ImVec2(Position.x + Size.x, Position.y + Size.y), Tint & 0x55FFFFFF,
                                2.0f);
    }
}

void VapeGui::ResetPositions() {
    MainX = 12.0f;
    MainY = Vape::PanelTop;
    for (auto& Category : Categories) {
        Category.PositionX = Category.DefaultX;
        Category.PositionY = Category.DefaultY;
    }
    FriendsX = FriendsDefaultX;
    FriendsY = FriendsDefaultY;

    SearchX = SearchY = -1.0f;
    DraggingSearch = false;
}

void VapeGui::PollKeybinds() {
    if (BindingModule) {
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0 && !KeysDownAtCapture[VK_ESCAPE])
            BindingModule = nullptr;
        else {
            for (int I = 0; I < 52; ++I) {
                int Vk = GetBindableKey(I);
                if (Vk <= 0 || Vk >= 256)
                    continue;
                if ((GetAsyncKeyState(Vk) & 0x8000) != 0 && !KeysDownAtCapture[Vk]) {
                    BindingModule->BindKey = Vk;
                    BindingModule->HoldToEnable = false;
                    BindingModule->WasBindDown = true;
                    Notify(BindingModule->Name.c_str(), ("Bound to " + GetKeyName(Vk)).c_str(), 1.5f);
                    BindingModule = nullptr;
                    break;
                }
            }
        }
    }
    if (BindingModule || ImGui::GetIO().WantTextInput)
        return;
    for (auto& Category : Categories) {
        for (auto& Module : Category.Modules) {
            if (Module.BindKey <= 0)
                continue;
            bool Down = (GetAsyncKeyState(Module.BindKey) & 0x8000) != 0;
            if (Module.HoldToEnable) {
                if (Module.Enabled != Down) {
                    Module.Enabled = Down;
                    if (ToggleNotifications)
                        NotifyModuleState(Module);
                }
            } else if (Down && !Module.WasBindDown) {
                Module.Enabled = !Module.Enabled;
                if (ToggleNotifications)
                    NotifyModuleState(Module);
            }
            Module.WasBindDown = Down;
        }
    }
}

void VapeGui::Render() {
    AnimationPending = false;
    HitRects.clear();

    PollKeybinds();

    RenderTextGui();
    RenderNotifications();

    if (!MainOpen)
        return;

    Vape::Category* LegitCategory = FindCategory("Legit");
    if (LegitCategory && LegitCategory->Open) {
        RenderLegit(*LegitCategory);
        return;
    }
    LegitSettings = nullptr;
    RenderMain();
    RenderSearch();
    for (auto& Category : Categories) {
        if (Category.IsLegit)
            continue;
        if (Category.Open)
            RenderCategory(Category);
    }
    if (FriendsOpen)
        RenderFriends();
    if (SettingsOpen)
        RenderSettings();
}

void VapeGui::NotifyModuleState(const Vape::Module& Module) {
    Notify(Module.Name, Module.Enabled ? "Enabled" : "Disabled", 1.5f,
           Module.Enabled ? Vape::NotificationKind::Info : Vape::NotificationKind::Warning);
}
