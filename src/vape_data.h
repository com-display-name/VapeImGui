// Demo data model for the Vape V4 clickgui clone.
// Categories -> Modules -> Options (Toggle / Slider / TwoSlider / Dropdown / TextBox)
#pragma once
#include <string>
#include <vector>

namespace vape {

enum class OptionType { Toggle, Slider, TwoSlider, Dropdown, TextBox };

struct ToggleOption {
    std::string name;
    bool enabled = false;
    std::string tooltip;
};

struct SliderOption {
    std::string name;
    float value = 50.0f;
    float minV = 0.0f;
    float maxV = 100.0f;
    float decimal = 1.0f; // rounding step denominator (1 = whole numbers)
    std::string suffix;
    std::string tooltip;
};

// TwoSlider.lua: one track with a min and a max handle (a value range).
struct TwoSliderOption {
    std::string name;
    float valueMin = 0.0f;
    float valueMax = 10.0f;
    float minV = 0.0f;    // track bound (props.Min)
    float maxV = 100.0f;  // track bound (props.Max)
    float decimal = 1.0f; // rounding step denominator (1 = whole numbers)
    char editText[16] = {0}; // label typing buffer
    int editSide = 0;        // 0 none, 1 min label, 2 max label
    std::string tooltip;
};

struct DropdownOption {
    std::string name;
    std::vector<std::string> items;
    int selected = 0;
    bool open = false;
    std::string tooltip;
};

struct TextBoxOption {
    std::string name;
    char text[64] = {0};
    std::string tooltip;
};

struct Option {
    OptionType type;
    ToggleOption toggle;
    SliderOption slider;
    TwoSliderOption twoslider;
    DropdownOption dropdown;
    TextBoxOption textbox;
};

struct Module {
    std::string name;
    std::string tooltip;
    bool enabled = false;
    bool expanded = false; // options visible
    bool visible = true;   // edit-mode hidden flag
    int index = 0;         // sort order / rainbow offset
    std::vector<Option> options;
    // Keybind (Bind.lua, module variant): Win32 VK code, 0 = unbound.
    int bindKey = 0;
    bool bindHold = false;   // true = enable while held, false = toggle
    bool bindWasDown = false; // previous physical state (edge detection)
};

struct FriendEntry {
    std::string name;
    bool enabled = true;
};

struct Category {
    std::string name;
    std::string iconFile; // e.g. "combat.png"
    // Legit Modules GUI: lives behind the search-bar legit switch, not a
    // main-window tab (Vape setting "Show Legit Mode").
    bool isLegit = false;
    bool open = false;    // category window visible
    bool expanded = true; // module list expanded
    float posX = 240.0f, posY = 60.0f;
    float defX = 240.0f, defY = 60.0f; // default position (reset target)
    std::vector<Module> modules;
};

struct Notification {
    std::string title;
    std::string text;
    float created = 0.0f;
    float duration = 3.0f;
    int kind = 0; // 0 info, 1 warning, 2 alert
};

} // namespace vape
