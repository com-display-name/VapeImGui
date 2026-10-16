// Vape V4 clickgui clone rendered with Dear ImGui.
// Mirrors layout/behavior of src/guis/new/{GUI,Category,Module,Toggle,Slider,...}.lua
//
// Host model: a single fullscreen overlay window. Panels are positioned at
// stored coords (relative to the selected monitor's origin). Empty areas are
// click-through via exact panel-rect hit-testing.
#pragma once
#include "imgui.h"
#include "vape_data.h"
#include "vape_style.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

struct ID3D11ShaderResourceView;

struct VapeIcon {
    ID3D11ShaderResourceView* srv = nullptr;
    int w = 0, h = 0;
};

struct HitRect {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

class VapeGui {
public:
    vape::Accent accent;
    std::vector<vape::Category> categories;

    // Panel positions, relative to the selected monitor's origin.
    float mainX = 12.0f, mainY = vape::kRowTopY; // top-aligned with the category row
    bool mainOpen = true;
    // Legit Modules GUI window (LegitWindow.lua): a standalone window that
    // replaces the whole main GUI while open (search-bar legit switch).
    float legitX = -1.0f, legitY = -1.0f;  // < 0 = centre on next open
    char legitBuf[64] = {0};               // "Search mods" filter
    vape::Module* legitSettings = nullptr; // module with the options pane open
    bool settingsOpen = false;
    bool editMode = false;
    bool textGuiEnabled = true;
    bool toggleNotifications = true;
    char searchBuf[64] = {0};
    // Floating search bar placement; <0 = centre at the top on the next frame
    // (startup / Reset positions). searchDrag is hold-to-move state - a plain
    // click still just focuses the field (see RenderSearch).
    float searchX = -1.0f, searchY = -1.0f;
    bool searchDrag = false;

    std::map<std::string, VapeIcon> icons;
    std::vector<vape::Notification> notifications;

    // Interactive panel rects in overlay client coords, rebuilt every frame.
    // The host hit-tests these for click-through (never trust hover state).
    std::vector<HitRect> hitRects;

    // Overlay display selection (populated by main.cpp monitor enumeration).
    int monitorIndex = 0;
    std::vector<std::string> monitorNames;
    bool monitorDropdownOpen = false;
    bool alwaysOnTop = true;
    // Screenproof: the overlay is omitted from screen capture/sharing
    // (SetWindowDisplayAffinity WDA_EXCLUDEFROMCAPTURE) but stays visible
    // on the monitor.
    bool screenproof = false;
    std::function<void(int)> onMonitorChange;
    std::function<void(bool)> onTopmostChange;
    std::function<void(bool)> onScreenproofChange;

    // Friends list window (CategoryList.lua): player list with add box,
    // per-entry enable dot + remove button, and a side options pane.
    bool friendsOpen = false;
    bool friendsExpanded = true;
    bool friendsOptionsOpen = false;
    float friendsX = 988.0f, friendsY = 260.0f;
    float friendsDefX = 988.0f, friendsDefY = 260.0f;
    std::vector<vape::FriendEntry> friends;
    char friendBuf[64] = {0};
    bool friendsRecolor = true;
    bool friendsUse = true;
    float friendsHue = 0.46f, friendsSat = 0.96f, friendsVal = 0.52f;

    // Module currently capturing a keybind (Bind.lua, module variant).
    vape::Module* bindingModule = nullptr;
    // TwoSlider drag state: 1 = min grip, 2 = max grip (one drag at a time).
    int twoSliderDrag = 0;
    bool bindCaptureSeed[256] = {false};

    // Settings sub-panes (branch off the Settings pane like vape SettingsPanes).
    // Branch navigation: settingsPage 0 = root, 1 = Visual, 2 = Debug.
    // The pane swaps its entire content to the sub-page (no inline dropdown).
    int settingsPage = 0;
    bool visualOpen = false;
    bool debugOpen = false;
    float visualX = 250.0f, visualY = 332.0f;
    float debugX = 488.0f, debugY = 12.0f;

    // "Fullscreen" inset mode (Settings > Debug): holder leaves a 32px top
    // bar plus the taskbar area uncovered instead of true fullscreen.
    bool fullscreen = false;
    std::function<void(bool)> onFullscreenChange;

    bool showConsole = false;
    std::function<void(bool)> onConsoleChange;

    void ResetPositions();

    void InitDemoData();
    void Notify(const std::string& title, const std::string& text, float duration = 3.0f, int kind = 0);
    void SetIcon(const std::string& name, ID3D11ShaderResourceView* srv, int w, int h);

    // Panel sizes (host sizes each window exactly to its panel).
    ImVec2 MainSize() const;
    ImVec2 CategorySize(const vape::Category& cat) const;
    ImVec2 SettingsSize() const;
    ImVec2 VisualSize() const;
    ImVec2 DebugSize() const;
    vape::Category* FindCategory(const std::string& name);

    // One slice per panel (all render into the single overlay window).
    // Each slice draws at its stored position; the host is fullscreen.
    void Render();
    void RenderMain();
    void RenderSearch(); // floating top-centre search bar (SearchBar.lua)
    void RenderLegit(vape::Category& cat); // Legit Modules GUI (LegitWindow.lua)
    void RenderCategory(vape::Category& cat);
    void RenderFriends(); // friends list window (CategoryList.lua)
    ImVec2 FriendsSize() const;
    void RenderSettings();
    void RenderVisual();
    void RenderDebug();

private:
    void RenderTextGui(const ImVec4& accentRgb);
    void RenderNotifications();
    void RenderModuleRow(vape::Category& cat, vape::Module& mod, const ImVec4& accentRgb, float t, float y, float& used);
    // When true, rows run full-bleed (0px inset); when false they stop 1px
    // short of the border (reverted size). Driven by the module's enabled state.
    bool rowFullBleed = false;
    void RenderToggleOption(vape::Module& mod, vape::ToggleOption& opt, const ImVec4& accentRgb, float y, float& used);
    void RenderSliderOption(vape::SliderOption& opt, const ImVec4& accentRgb, float y, float& used);
    void RenderTwoSliderOption(vape::TwoSliderOption& opt, const ImVec4& accentRgb, float y, float& used);
    void RenderDropdownOption(vape::DropdownOption& opt, float y, float& used);
    void RenderTextBoxOption(vape::TextBoxOption& opt, float y, float& used);

    // Settings-pane row widgets (shared by Settings/Visual/Debug panes).
    bool VapeToggleRow(const char* label, bool& v, const char* id, const ImVec4& accentRgb);
    bool VapeNavButton(const char* label, const char* id, const ImVec4& accentRgb);
    bool VapeActionButton(const char* label, const char* id);
    void VapeHSVSliders();
    void VapeFriendHueSlider(); // single hue row for the friends list color
    void VapePresetSwatches();
    void VapeDisplaySelector();
    void PollBinds(); // keybind capture (global) + bound-key triggers

    float OptionHeight(const vape::Option& o) const;
    float ModuleHeight(const vape::Module& m) const;

    void DrawIcon(const char* name, ImVec2 pos, ImVec2 size, ImU32 tint);
    // Tooltip padding: 3px left/right, 2px top/bottom.
    void ShowTooltip(const char* text);
};

extern VapeGui g_vape;
