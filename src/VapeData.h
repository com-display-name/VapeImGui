#pragma once
#include <string>
#include <vector>

namespace Vape {
enum class SettingsView {
    Root,
    Visual,
    Debug
};
enum class NotificationKind {
    Info,
    Warning,
    Alert
};

enum class OptionType {
    Toggle,
    Slider,
    TwoSlider,
    Dropdown,
    TextBox
};

struct ToggleOption {
    std::string Name;
    bool Enabled = false;
    std::string Tooltip;
};

struct SliderOption {
    std::string Name;
    float Value = 50.0f;
    float Minimum = 0.0f;
    float Maximum = 100.0f;
    float RoundingScale = 1.0f; // 10 rounds down to tenths.
    std::string Suffix;
    std::string Tooltip;
};

struct TwoSliderOption {
    std::string Name;
    float LowerValue = 0.0f;
    float UpperValue = 10.0f;
    float Minimum = 0.0f;
    float Maximum = 100.0f;
    float RoundingScale = 1.0f; // 10 rounds down to tenths.
    std::string Tooltip;
};

struct DropdownOption {
    std::string Name;
    std::vector<std::string> Items;
    int Selected = 0;
    bool Open = false;
    std::string Tooltip;
};

struct TextBoxOption {
    std::string Name;
    char Text[64] = {0};
    std::string Tooltip;
};

struct Option {
    OptionType Type = OptionType::Toggle;
    ToggleOption Toggle;
    SliderOption Slider;
    TwoSliderOption TwoSlider;
    DropdownOption Dropdown;
    TextBoxOption TextBox;
};

struct Module {
    std::string Name;
    std::string Tooltip;
    bool Enabled = false;
    bool Expanded = false;
    bool Visible = true;
    int Index = 0;
    std::vector<Option> Options;

    int BindKey = 0; // Win32 virtual key; zero is unbound.
    bool HoldToEnable = false;
    bool WasBindDown = false;
};

struct FriendEntry {
    std::string Name;
    bool Enabled = true;
};

struct Category {
    std::string Name;
    std::string IconFile;

    bool IsLegit = false;
    bool Open = false;
    bool Expanded = true;
    float PositionX = 240.0f, PositionY = 60.0f;
    float DefaultX = 240.0f, DefaultY = 60.0f;
    std::vector<Module> Modules;
};

struct Notification {
    std::string Title;
    std::string Text;
    float Created = 0.0f;
    float Duration = 3.0f;
    Vape::NotificationKind Kind = Vape::NotificationKind::Info;
};

} // namespace Vape
