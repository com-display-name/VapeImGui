#include "VapeGui.h"
#include "VapeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::InitializeModules() {
    using namespace Vape;
    Categories.clear();

    auto MakeToggle = [](const char* Name, bool DefaultValue = false, const char* Tooltip = "") {
        Option Result;
        Result.Type = OptionType::Toggle;
        Result.Toggle.Name = Name;
        Result.Toggle.Enabled = DefaultValue;
        Result.Toggle.Tooltip = Tooltip;
        return Result;
    };
    auto MakeSlider = [](const char* Name, float DefaultValue, float Minimum, float Maximum,
                         const char* Suffix = "", float RoundingScale = 1.0f) {
        Option Result;
        Result.Type = OptionType::Slider;
        Result.Slider.Name = Name;
        Result.Slider.Value = DefaultValue;
        Result.Slider.Minimum = Minimum;
        Result.Slider.Maximum = Maximum;
        Result.Slider.Suffix = Suffix;
        Result.Slider.RoundingScale = RoundingScale;
        return Result;
    };
    auto MakeTwoSlider = [](const char* Name, float DefaultMinimum, float DefaultMaximum, float Minimum,
                            float Maximum, float RoundingScale = 1.0f) {
        Option Result;
        Result.Type = OptionType::TwoSlider;
        Result.TwoSlider.Name = Name;
        Result.TwoSlider.LowerValue = DefaultMinimum;
        Result.TwoSlider.UpperValue = DefaultMaximum;
        Result.TwoSlider.Minimum = Minimum;
        Result.TwoSlider.Maximum = Maximum;
        Result.TwoSlider.RoundingScale = RoundingScale;
        return Result;
    };
    auto MakeDropdown = [](const char* Name, std::vector<std::string> Items, int Selected = 0) {
        Option Result;
        Result.Type = OptionType::Dropdown;
        Result.Dropdown.Name = Name;
        Result.Dropdown.Items = std::move(Items);
        Result.Dropdown.Selected = Selected;
        return Result;
    };
    auto MakeTextBox = [](const char* Name, const char* DefaultValue = "") {
        Option Result;
        Result.Type = OptionType::TextBox;
        Result.TextBox.Name = Name;
        snprintf(Result.TextBox.Text, sizeof(Result.TextBox.Text), "%s", DefaultValue);
        return Result;
    };
    auto MakeModule = [](const char* Name, const char* Tooltip = "") {
        Module CurrentModule;
        CurrentModule.Name = Name;
        CurrentModule.Tooltip = Tooltip;
        return CurrentModule;
    };

    int NextModuleIndex = 0;
    auto AssignModuleIndex = [&NextModuleIndex](Module& CurrentModule) {
        CurrentModule.Index = NextModuleIndex++;
    };

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Combat";
        CurrentCategory.IconFile = "combat.png";
        CurrentCategory.Open = true;
        CurrentCategory.Expanded = true;
        Module Sprint = MakeModule("Sprint", "Keeps you sprinting");
        Sprint.Options = {MakeToggle("Particles"), MakeToggle("Only in game"),
                          MakeSlider("Speed", 100, 0, 100, "%")};
        Module Velocity = MakeModule("Velocity", "Modifies knockback");
        Velocity.Options = {MakeSlider("Horizontal", 90, 0, 100, "%"),
                            MakeSlider("Vertical", 100, 0, 100, "%"), MakeSlider("Chance", 100, 0, 100, "%"),
                            MakeDropdown("Mode", {"Cancel", "Jump reset", "Delayed"})};
        Module Reach = MakeModule("Reach", "Extends attack range");
        Reach.Options = {MakeTwoSlider("Range", 3.0f, 4.5f, 3.0f, 6.0f, 10.0f),
                         MakeToggle("Only while sprinting")};
        Module HitBoxes = MakeModule("HitBoxes", "Expands player hitboxes");
        HitBoxes.Options = {MakeToggle("Expand"), MakeSlider("Size", 2, 1, 10, "x", 10.0f)};
        for (auto* CurrentModule : {&Sprint, &Velocity, &Reach, &HitBoxes}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Blatant";
        CurrentCategory.IconFile = "blatant.png";
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module Speed = MakeModule("Speed", "Move faster");
        Speed.Options = {MakeDropdown("Mode", {"CFrame", "Velocity", "Pulse"}),
                         MakeSlider("Speed", 23, 0, 100), MakeToggle("Strafe")};
        Module Fly = MakeModule("Fly", "Fly around the map");
        Fly.Options = {MakeDropdown("Mode", {"CFrame", "Velocity", "Jump"}), MakeSlider("Speed", 50, 0, 150),
                       MakeSlider("Vertical", 50, 0, 150)};
        Module NoFall = MakeModule("NoFall", "Prevents fall damage");
        NoFall.Options = {MakeDropdown("Mode", {"Spoof", "Land", "Cancel"}), MakeToggle("Notify")};
        Module LongJump = MakeModule("LongJump", "Jump further");
        LongJump.Options = {MakeSlider("Power", 120, 0, 300, "%"), MakeSlider("Up", 40, 0, 100, "%"),
                            MakeDropdown("Mode", {"Jump", "Double", "Repeated"})};
        for (auto* CurrentModule : {&Speed, &Fly, &NoFall, &LongJump}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Render";
        CurrentCategory.IconFile = "render.png";
        CurrentCategory.Open = true;
        CurrentCategory.Expanded = true;
        Module ESP = MakeModule("ESP", "See players through walls");
        ESP.Options = {MakeToggle("Players", true), MakeToggle("Chests"), MakeToggle("Ores"),
                       MakeSlider("Max distance", 1000, 100, 5000, " studs")};
        Module Chams = MakeModule("Chams", "Highlight models");
        Chams.Options = {MakeToggle("Players", true), MakeToggle("Through walls", true),
                         MakeDropdown("Material", {"Flat", "Neon", "Wireframe"})};
        Module Tracers = MakeModule("Tracers", "Lines to entities");
        Tracers.Enabled = true;
        Tracers.Options = {MakeToggle("Players", true), MakeSlider("Thickness", 1, 1, 5, "px")};
        Module TargetHUD = MakeModule("TargetHUD", "Target info overlay");
        TargetHUD.Options = {MakeToggle("Background"), MakeDropdown("Position", {"Top", "Middle", "Mouse"})};
        for (auto* CurrentModule : {&ESP, &Chams, &Tracers, &TargetHUD}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Utility";
        CurrentCategory.IconFile = "utility.png";
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module AutoClicker = MakeModule("AutoClicker", "Clicks for you");
        AutoClicker.Options = {MakeTwoSlider("CPS", 10, 12, 1, 20), MakeToggle("Break blocks"),
                               MakeToggle("Only while holding")};
        Module ChestStealer = MakeModule("ChestStealer", "Loots chests fast");
        ChestStealer.Options = {MakeSlider("Delay", 100, 0, 500, "ms"), MakeToggle("Randomize")};
        Module Refill = MakeModule("Refill", "Refills hotbar items");
        Refill.Options = {MakeToggle("Blocks"), MakeToggle("Healing"),
                          MakeSlider("Delay", 500, 0, 2000, "ms")};
        Module NoSlow = MakeModule("NoSlow", "No slowdown on use");
        NoSlow.Options = {MakeSlider("Slow", 0, 0, 100, "%"), MakeDropdown("Mode", {"Spoof", "Cancel"})};
        for (auto* CurrentModule : {&AutoClicker, &ChestStealer, &Refill, &NoSlow}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "World";
        CurrentCategory.IconFile = "world.png";
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module TimeChanger = MakeModule("TimeChanger", "Changes world time");
        TimeChanger.Options = {MakeSlider("Time", 14, 0, 24, "h"), MakeToggle("Smooth")};
        Module AntiVoid = MakeModule("AntiVoid", "Saves you from the void");
        AntiVoid.Enabled = true;
        AntiVoid.Options = {MakeSlider("Height", 5, -20, 30, " studs"),
                            MakeDropdown("Mode", {"Teleport", "Jump", "Fly"})};
        Module Timer = MakeModule("Timer", "Changes game speed");
        Timer.Options = {MakeSlider("Speed", 110, 10, 300, "%")};
        Module AutoLeave = MakeModule("AutoLeave", "Leaves on staff join");
        AutoLeave.Options = {MakeTextBox("Keyword", "mod"), MakeToggle("Notify", true)};
        for (auto* CurrentModule : {&TimeChanger, &AntiVoid, &Timer, &AutoLeave}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Inventory";
        CurrentCategory.IconFile = "inventory.png";
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module AutoArmor = MakeModule("AutoArmor", "Equips best armor");
        AutoArmor.Options = {MakeToggle("Prioritize", true), MakeSlider("Delay", 200, 0, 1000, "ms")};
        Module AutoHotbar = MakeModule("AutoHotbar", "Manages hotbar slots");
        AutoHotbar.Options = {MakeDropdown("Slot", {"1", "2", "3", "4", "5", "6", "7", "8", "9"}),
                              MakeToggle("Sword first")};
        Module ChestSort = MakeModule("ChestSort", "Sorts chest contents");
        ChestSort.Options = {MakeToggle("Auto stack"), MakeToggle("Trash junk")};
        for (auto* CurrentModule : {&AutoArmor, &AutoHotbar, &ChestSort}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Minigames";
        CurrentCategory.IconFile = "friends.png";
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module AutoBridge = MakeModule("AutoBridge", "Bridges for you");
        AutoBridge.Options = {MakeToggle("Sneak", true), MakeSlider("Delay", 100, 0, 500, "ms"),
                              MakeDropdown("Mode", {"Godbridge", "Moon", "Breezily"})};
        Module KillAura = MakeModule("KillAura", "Attacks nearby players");
        KillAura.Options = {MakeSlider("Range", 14, 5, 20, " studs"), MakeSlider("CPS", 10, 1, 20),
                            MakeToggle("Require sword")};
        for (auto* CurrentModule : {&AutoBridge, &KillAura}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    {
        Category CurrentCategory;
        CurrentCategory.Name = "Legit";
        CurrentCategory.IconFile = "legit_mode_icon.png";
        CurrentCategory.IsLegit = true;
        CurrentCategory.Open = false;
        CurrentCategory.Expanded = true;
        Module Atmosphere = MakeModule("Atmosphere", "Custom lighting objects");
        Atmosphere.Options = {MakeDropdown("Preset", {"Night", "Sunset", "Storm"})};
        Module Breadcrumbs = MakeModule("Breadcrumbs", "Shows a trail behind your character");
        Breadcrumbs.Options = {MakeSlider("Length", 20, 5, 50), MakeToggle("Rainbow", true)};
        Module Cape = MakeModule("Cape", "Adds a cape to your character");
        Cape.Options = {MakeDropdown("Design", {"Classic", "Vape", "Wings"}), MakeToggle("Animated", true)};
        Module ChinaHat = MakeModule("ChinaHat", "Puts a china hat on your character");
        ChinaHat.Options = {MakeToggle("Rainbow", true), MakeSlider("Height", 10, 4, 20, "px")};
        Module CleanKit = MakeModule("CleanKit", "Removes the kit status indicator");
        Module Clock = MakeModule("Clock", "Shows the current time");
        Clock.Options = {MakeDropdown("Format", {"24h", "12h"}), MakeToggle("Seconds")};
        Module Crosshair = MakeModule("Crosshair", "Custom first person crosshair");
        Crosshair.Options = {MakeDropdown("Style", {"Cross", "Dot", "Circle"}), MakeToggle("Outline", true)};
        Module DamageIndicator = MakeModule("DamageIndicator", "Customize the damage indicator");
        DamageIndicator.Options = {MakeToggle("Outline", true), MakeToggle("Fade out", true)};
        Module Disguise = MakeModule("Disguise", "Changes your character or animation to an ID");
        Disguise.Options = {MakeTextBox("ID", "rbxassetid://")};
        Module FOV = MakeModule("FOV", "Adjusts camera vision");
        FOV.Options = {MakeSlider("Value", 90, 60, 120, " deg")};
        Module FPS = MakeModule("FPS", "Shows the current framerate");
        FPS.Options = {MakeToggle("Graph")};
        Module FPSBoost = MakeModule("FPSBoost", "Improves framerate by turning off effects");
        FPSBoost.Options = {MakeToggle("Low graphics", true), MakeToggle("No particles", true)};
        Module HitColor = MakeModule("HitColor", "Customize the hit highlight color");
        HitColor.Options = {MakeToggle("Rainbow")};
        Module HitFix = MakeModule("HitFix", "Changes the raycast function to the correct one");
        Module Interface = MakeModule("Interface", "Customize bedwars UI");
        Interface.Options = {MakeDropdown("Style", {"Default", "Compact"})};
        Module Keystrokes = MakeModule("Keystrokes", "Shows movement keys onscreen");
        Keystrokes.Options = {MakeToggle("WASD", true), MakeToggle("CPS", true)};
        Module KillEffect = MakeModule("KillEffect", "Custom final kill effects");
        KillEffect.Options = {MakeDropdown("Effect", {"Flame", "Lightning", "Ghost"})};
        Module Memory = MakeModule("Memory", "Shows the memory used by Roblox");
        Memory.Options = {MakeToggle("Graph")};
        Module Ping = MakeModule("Ping", "Shows the current connection speed");
        Ping.Options = {MakeToggle("Graph")};
        Module ReachDisplay = MakeModule("ReachDisplay", "Shows the reach you hit from");
        ReachDisplay.Options = {MakeToggle("Show max")};
        Module SongBeats = MakeModule("SongBeats", "Built in mp3 player");
        SongBeats.Options = {MakeTextBox("Song ID", "rbxassetid://"), MakeSlider("Volume", 50, 0, 100, "%")};
        Module Speedmeter = MakeModule("Speedmeter", "Shows your average velocity in studs");
        Speedmeter.Options = {MakeDropdown("Unit", {"Studs/s", "MPH", "KM/H"})};
        Module TimeChanger = MakeModule("TimeChanger", "Changes the time of the current world");
        TimeChanger.Options = {MakeSlider("Time", 14, 0, 24, "h"), MakeToggle("Smooth", true)};
        Module UICleanup = MakeModule("UICleanup", "Cleans up the UI for kits & main");
        UICleanup.Options = {MakeToggle("Kit UI", true), MakeToggle("Leaderboard")};
        Module Viewmodel = MakeModule("Viewmodel", "Changes the viewmodel animations");
        Viewmodel.Options = {MakeToggle("Custom anims", true), MakeSlider("FOV offset", 10, -30, 30, " deg")};
        for (auto* CurrentModule :
             {&Atmosphere,      &Breadcrumbs, &Cape,       &ChinaHat, &CleanKit, &Clock,        &Crosshair,
              &DamageIndicator, &Disguise,    &FOV,        &FPS,      &FPSBoost, &HitColor,     &HitFix,
              &Interface,       &Keystrokes,  &KillEffect, &Memory,   &Ping,     &ReachDisplay, &SongBeats,
              &Speedmeter,      &TimeChanger, &UICleanup,  &Viewmodel}) {
            AssignModuleIndex(*CurrentModule);
            CurrentCategory.Modules.push_back(*CurrentModule);
        }
        Categories.push_back(std::move(CurrentCategory));
    }

    FriendsOpen = false;
    FriendsExpanded = true;
    FriendsOptionsOpen = false;
    Friends.clear();
    FriendNameBuffer[0] = '\0';
    FriendsRecolor = true;
    FriendsUse = true;
    FriendsHue = 0.46f;
    FriendsSaturation = 0.96f;
    FriendsValue = 0.52f;
    BindingModule = nullptr;
    for (int I = 0; I < 256; ++I)
        KeysDownAtCapture[I] = false;

    const float RowY = PanelTop;
    float RowX = 256.0f;
    for (auto& CurrentCategory : Categories) {
        if (CurrentCategory.IsLegit)
            continue;
        CurrentCategory.PositionX = RowX;
        CurrentCategory.PositionY = RowY;
        RowX += PanelWidth + 16.0f;
    }

    FriendsX = FriendsDefaultX = RowX;
    FriendsY = FriendsDefaultY = RowY;

    for (auto& CurrentCategory : Categories) {
        CurrentCategory.DefaultX = CurrentCategory.PositionX;
        CurrentCategory.DefaultY = CurrentCategory.PositionY;
    }
    ResetPositions();
}
