#pragma once
#include "VapeData.h"
#include "VapeStyle.h"
#include "imgui.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

struct ID3D11ShaderResourceView;

struct VapeIcon {
    ID3D11ShaderResourceView* ShaderResource = nullptr;
    int Width = 0, Height = 0;
};

struct HitRect {
    float X0 = 0, Y0 = 0, X1 = 0, Y1 = 0;
};

class VapeGui {
  public:
    Vape::Accent Accent;
    std::vector<Vape::Category> Categories;

    float MainX = 12.0f, MainY = Vape::PanelTop;
    bool MainOpen = true;

    float LegitX = -1.0f, LegitY = -1.0f;
    char LegitSearchBuffer[64] = {0};
    Vape::Module* LegitSettings = nullptr;
    bool SettingsOpen = false;
    bool EditMode = false;
    bool TextGuiEnabled = true;
    bool ToggleNotifications = true;
    char SearchBuffer[64] = {0};

    float SearchX = -1.0f, SearchY = -1.0f;
    bool DraggingSearch = false;

    std::map<std::string, VapeIcon> Icons;
    std::vector<Vape::Notification> Notifications;

    std::vector<HitRect> HitRects;

    int MonitorIndex = 0;
    std::vector<std::string> MonitorNames;
    bool MonitorDropdownOpen = false;
    bool AlwaysOnTop = true;

    bool Streamproof = false;
    std::function<void(int)> OnMonitorChange;
    std::function<void(bool)> OnTopmostChange;
    std::function<void(bool)> OnStreamproofChange;

    bool FriendsOpen = false;
    bool FriendsExpanded = true;
    bool FriendsOptionsOpen = false;
    float FriendsX = 988.0f, FriendsY = 260.0f;
    float FriendsDefaultX = 988.0f, FriendsDefaultY = 260.0f;
    std::vector<Vape::FriendEntry> Friends;
    char FriendNameBuffer[64] = {0};
    bool FriendsRecolor = true;
    bool FriendsUse = true;
    float FriendsHue = 0.46f, FriendsSaturation = 0.96f, FriendsValue = 0.52f;

    Vape::Module* BindingModule = nullptr;

    int DraggedRangeHandle = 0;
    bool KeysDownAtCapture[256] = {false};

    Vape::SettingsView SettingsPage = Vape::SettingsView::Root;

    bool Fullscreen = false;
    std::function<void(bool)> OnFullscreenChange;

    void ResetPositions();

    void InitializeModules();
    void Notify(const std::string& Title, const std::string& Text, float Duration = 3.0f,
                Vape::NotificationKind Kind = Vape::NotificationKind::Info);
    void SetIcon(const std::string& Name, ID3D11ShaderResourceView* ShaderResource, int Width, int Height);
    void ShowTooltip(const char* Text, float Delay = 0.5f);
    bool IsAnimating() const;

    ImVec2 GetMainSize() const;
    ImVec2 GetCategorySize(const Vape::Category& Category) const;
    ImVec2 GetSettingsSize() const;
    Vape::Category* FindCategory(const std::string& Name);

    void Render();
    void RenderMain();
    void RenderSearch();
    void RenderLegit(Vape::Category& Category);
    void RenderCategory(Vape::Category& Category);
    void RenderFriends();
    ImVec2 GetFriendsSize() const;
    void RenderSettings();

  private:
    struct ScrollAnimation {
        ImGuiID WindowId = 0;
        float Position = 0.0f;
        float Target = 0.0f;
        int LastFrame = -1;
    };

    std::map<ImGuiID, ScrollAnimation> ScrollAnimations;
    bool AnimationPending = false;
    ImGuiID TooltipId = 0;
    int TooltipFrame = -1;
    double TooltipStarted = 0.0;
    ImVec2 TooltipPosition;
    bool TooltipPositionValid = false;

    void BeginScrollableChild(const char* Id, ImVec2 Size);
    void RenderTextGui();
    void RenderNotifications();
    void RenderModuleRow(Vape::Module& Module, float Time);

    void NotifyModuleState(const Vape::Module& Module);
    void RenderModuleOptions(Vape::Module& Module, const ImVec4& AccentColor);
    void RenderToggleOption(Vape::Module& Module, Vape::ToggleOption& Option, const ImVec4& AccentColor);
    void RenderSliderOption(Vape::SliderOption& Option, const ImVec4& AccentColor);
    void RenderTwoSliderOption(Vape::TwoSliderOption& Option, const ImVec4& AccentColor);
    void RenderDropdownOption(Vape::DropdownOption& Option);
    void RenderTextBoxOption(Vape::TextBoxOption& Option);

    bool RenderToggleRow(const char* Label, bool& Value, const char* WidgetId, const ImVec4& AccentColor);
    bool RenderNavigationButton(const char* Label, const char* WidgetId);
    bool RenderActionButton(const char* Label, const char* WidgetId);
    void RenderAccentSliders();
    void RenderFriendHueSlider();
    void RenderPresetSwatches();
    void RenderDisplaySelector();
    void PollKeybinds();

    float GetOptionHeight(const Vape::Option& Option) const;
    float GetModuleHeight(const Vape::Module& Module) const;

    void DrawIcon(const char* Name, ImVec2 Position, ImVec2 Size, ImU32 Tint);
};

extern VapeGui Gui;
