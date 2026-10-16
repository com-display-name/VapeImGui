// Module catalogue: every category and module the GUI shows (InitDemoData),
// built once at startup.
#include "vape_gui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "vape_internal.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>

void VapeGui::InitDemoData() {
    using namespace vape;
    categories.clear();

    auto toggle = [](const char* n, bool def = false, const char* tip = "") {
        Option o; o.type = OptionType::Toggle;
        o.toggle.name = n; o.toggle.enabled = def; o.toggle.tooltip = tip;
        return o;
    };
    auto slider = [](const char* n, float def, float mn, float mx, const char* suf = "", float dec = 1.0f) {
        Option o; o.type = OptionType::Slider;
        o.slider.name = n; o.slider.value = def;
        o.slider.minV = mn; o.slider.maxV = mx;
        o.slider.suffix = suf; o.slider.decimal = dec;
        return o;
    };
    auto twoslider = [](const char* n, float defMin, float defMax, float mn, float mx, float dec = 1.0f) {
        Option o; o.type = OptionType::TwoSlider;
        o.twoslider.name = n;
        o.twoslider.valueMin = defMin; o.twoslider.valueMax = defMax;
        o.twoslider.minV = mn; o.twoslider.maxV = mx;
        o.twoslider.decimal = dec;
        return o;
    };
    auto dropdown = [](const char* n, std::vector<std::string> items, int sel = 0) {
        Option o; o.type = OptionType::Dropdown;
        o.dropdown.name = n; o.dropdown.items = std::move(items); o.dropdown.selected = sel;
        return o;
    };
    auto textbox = [](const char* n, const char* def = "") {
        Option o; o.type = OptionType::TextBox;
        o.textbox.name = n;
        snprintf(o.textbox.text, sizeof(o.textbox.text), "%s", def);
        return o;
    };
    auto module = [](const char* n, const char* tip = "") {
        Module m; m.name = n; m.tooltip = tip;
        return m;
    };

    int idx = 0;
    auto finishMod = [&idx](Module& m) { m.index = idx++; };

    // ---- Combat ----
    {
        Category c; c.name = "Combat"; c.iconFile = "combat.png"; c.open = true; c.expanded = true;
        Module m1 = module("Sprint", "Keeps you sprinting");
        m1.options = { toggle("Particles"), toggle("Only in game"), slider("Speed", 100, 0, 100, "%") };
        Module m2 = module("Velocity", "Modifies knockback");
        m2.options = { slider("Horizontal", 90, 0, 100, "%"), slider("Vertical", 100, 0, 100, "%"), slider("Chance", 100, 0, 100, "%"), dropdown("Mode", {"Cancel", "Jump reset", "Delayed"}) };
        Module m3 = module("Reach", "Extends attack range");
        m3.options = { twoslider("Range", 3.0f, 4.5f, 3.0f, 6.0f, 10.0f), toggle("Only while sprinting") };
        Module m4 = module("HitBoxes", "Expands player hitboxes");
        m4.options = { toggle("Expand"), slider("Size", 2, 1, 10, "x", 10.0f) };
        for (auto* m : {&m1, &m2, &m3, &m4}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Blatant ----
    {
        Category c; c.name = "Blatant"; c.iconFile = "blatant.png"; c.open = false; c.expanded = true;
        Module m1 = module("Speed", "Move faster");
        m1.options = { dropdown("Mode", {"CFrame", "Velocity", "Pulse"}), slider("Speed", 23, 0, 100), toggle("Strafe") };
        Module m2 = module("Fly", "Fly around the map");
        m2.options = { dropdown("Mode", {"CFrame", "Velocity", "Jump"}), slider("Speed", 50, 0, 150), slider("Vertical", 50, 0, 150) };
        Module m3 = module("NoFall", "Prevents fall damage");
        m3.options = { dropdown("Mode", {"Spoof", "Land", "Cancel"}), toggle("Notify") };
        Module m4 = module("LongJump", "Jump further");
        m4.options = { slider("Power", 120, 0, 300, "%"), slider("Up", 40, 0, 100, "%"), dropdown("Mode", {"Jump", "Double", "Repeated"}) };
        for (auto* m : {&m1, &m2, &m3, &m4}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Render ----
    {
        Category c; c.name = "Render"; c.iconFile = "render.png"; c.open = true; c.expanded = true;
        Module m1 = module("ESP", "See players through walls");
        m1.options = { toggle("Players", true), toggle("Chests"), toggle("Ores"), slider("Max distance", 1000, 100, 5000, " studs") };
        Module m2 = module("Chams", "Highlight models");
        m2.options = { toggle("Players", true), toggle("Through walls", true), dropdown("Material", {"Flat", "Neon", "Wireframe"}) };
        Module m3 = module("Tracers", "Lines to entities");
        m3.enabled = true;
        m3.options = { toggle("Players", true), slider("Thickness", 1, 1, 5, "px") };
        Module m4 = module("TargetHUD", "Target info overlay");
        m4.options = { toggle("Background"), dropdown("Position", {"Top", "Middle", "Mouse"}) };
        for (auto* m : {&m1, &m2, &m3, &m4}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Utility ----
    {
        Category c; c.name = "Utility"; c.iconFile = "utility.png"; c.open = false; c.expanded = true;
        Module m1 = module("AutoClicker", "Clicks for you");
        m1.options = { slider("CPS", 12, 1, 20), toggle("Break blocks"), toggle("Only while holding") };
        Module m2 = module("ChestStealer", "Loots chests fast");
        m2.options = { slider("Delay", 100, 0, 500, "ms"), toggle("Randomize") };
        Module m3 = module("Refill", "Refills hotbar items");
        m3.options = { toggle("Blocks"), toggle("Healing"), slider("Delay", 500, 0, 2000, "ms") };
        Module m4 = module("NoSlow", "No slowdown on use");
        m4.options = { slider("Slow", 0, 0, 100, "%"), dropdown("Mode", {"Spoof", "Cancel"}) };
        for (auto* m : {&m1, &m2, &m3, &m4}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- World ----
    {
        Category c; c.name = "World"; c.iconFile = "world.png"; c.open = false; c.expanded = true;
        Module m1 = module("TimeChanger", "Changes world time");
        m1.options = { slider("Time", 14, 0, 24, "h"), toggle("Smooth") };
        Module m2 = module("AntiVoid", "Saves you from the void");
        m2.enabled = true;
        m2.options = { slider("Height", 5, -20, 30, " studs"), dropdown("Mode", {"Teleport", "Jump", "Fly"}) };
        Module m3 = module("Timer", "Changes game speed");
        m3.options = { slider("Speed", 110, 10, 300, "%") };
        Module m4 = module("AutoLeave", "Leaves on staff join");
        m4.options = { textbox("Keyword", "mod"), toggle("Notify", true) };
        for (auto* m : {&m1, &m2, &m3, &m4}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Inventory ----
    {
        Category c; c.name = "Inventory"; c.iconFile = "inventory.png"; c.open = false; c.expanded = true;
        Module m1 = module("AutoArmor", "Equips best armor");
        m1.options = { toggle("Prioritize", true), slider("Delay", 200, 0, 1000, "ms") };
        Module m2 = module("AutoHotbar", "Manages hotbar slots");
        m2.options = { dropdown("Slot", {"1", "2", "3", "4", "5", "6", "7", "8", "9"}), toggle("Sword first") };
        Module m3 = module("ChestSort", "Sorts chest contents");
        m3.options = { toggle("Auto stack"), toggle("Trash junk") };
        for (auto* m : {&m1, &m2, &m3}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Minigames ----
    {
        Category c; c.name = "Minigames"; c.iconFile = "friends.png"; c.open = false; c.expanded = true;
        Module m1 = module("AutoBridge", "Bridges for you");
        m1.options = { toggle("Sneak", true), slider("Delay", 100, 0, 500, "ms"), dropdown("Mode", {"Godbridge", "Moon", "Breezily"}) };
        Module m2 = module("KillAura", "Attacks nearby players");
        m2.options = { slider("Range", 14, 5, 20, " studs"), slider("CPS", 10, 1, 20), toggle("Require sword") };
        for (auto* m : {&m1, &m2}) { finishMod(*m); c.modules.push_back(*m); }
        categories.push_back(std::move(c));
    }
    // ---- Legit (Legit Modules GUI) ----
    // Real Vape's separate legit mod menu, switched via the search-bar button
    // (setting "Show Legit Mode"). Module list from the reference's Legit Mods
    // section; not a main-window tab (isLegit).
    {
        Category c; c.name = "Legit"; c.iconFile = "legit_mode_icon.png";
        c.isLegit = true; c.open = false; c.expanded = true;
        Module m1 = module("Atmosphere", "Custom lighting objects");
        m1.options = { dropdown("Preset", {"Night", "Sunset", "Storm"}) };
        Module m2 = module("Breadcrumbs", "Shows a trail behind your character");
        m2.options = { slider("Length", 20, 5, 50), toggle("Rainbow", true) };
        Module m3 = module("Cape", "Adds a cape to your character");
        m3.options = { dropdown("Design", {"Classic", "Vape", "Wings"}), toggle("Animated", true) };
        Module m4 = module("ChinaHat", "Puts a china hat on your character");
        m4.options = { toggle("Rainbow", true), slider("Height", 10, 4, 20, "px") };
        Module m5 = module("CleanKit", "Removes the kit status indicator");
        Module m6 = module("Clock", "Shows the current time");
        m6.options = { dropdown("Format", {"24h", "12h"}), toggle("Seconds") };
        Module m7 = module("Crosshair", "Custom first person crosshair");
        m7.options = { dropdown("Style", {"Cross", "Dot", "Circle"}), toggle("Outline", true) };
        Module m8 = module("DamageIndicator", "Customize the damage indicator");
        m8.options = { toggle("Outline", true), toggle("Fade out", true) };
        Module m9 = module("Disguise", "Changes your character or animation to an ID");
        m9.options = { textbox("ID", "rbxassetid://") };
        Module m10 = module("FOV", "Adjusts camera vision");
        m10.options = { slider("Value", 90, 60, 120, " deg") };
        Module m11 = module("FPS", "Shows the current framerate");
        m11.options = { toggle("Graph") };
        Module m12 = module("FPSBoost", "Improves framerate by turning off effects");
        m12.options = { toggle("Low graphics", true), toggle("No particles", true) };
        Module m13 = module("HitColor", "Customize the hit highlight color");
        m13.options = { toggle("Rainbow") };
        Module m14 = module("HitFix", "Changes the raycast function to the correct one");
        Module m15 = module("Interface", "Customize bedwars UI");
        m15.options = { dropdown("Style", {"Default", "Compact"}) };
        Module m16 = module("Keystrokes", "Shows movement keys onscreen");
        m16.options = { toggle("WASD", true), toggle("CPS", true) };
        Module m17 = module("KillEffect", "Custom final kill effects");
        m17.options = { dropdown("Effect", {"Flame", "Lightning", "Ghost"}) };
        Module m18 = module("Memory", "Shows the memory used by Roblox");
        m18.options = { toggle("Graph") };
        Module m19 = module("Ping", "Shows the current connection speed");
        m19.options = { toggle("Graph") };
        Module m20 = module("ReachDisplay", "Shows the reach you hit from");
        m20.options = { toggle("Show max") };
        Module m21 = module("SongBeats", "Built in mp3 player");
        m21.options = { textbox("Song ID", "rbxassetid://"), slider("Volume", 50, 0, 100, "%") };
        Module m22 = module("Speedmeter", "Shows your average velocity in studs");
        m22.options = { dropdown("Unit", {"Studs/s", "MPH", "KM/H"}) };
        Module m23 = module("TimeChanger", "Changes the time of the current world");
        m23.options = { slider("Time", 14, 0, 24, "h"), toggle("Smooth", true) };
        Module m24 = module("UICleanup", "Cleans up the UI for kits & main");
        m24.options = { toggle("Kit UI", true), toggle("Leaderboard") };
        Module m25 = module("Viewmodel", "Changes the viewmodel animations");
        m25.options = { toggle("Custom anims", true), slider("FOV offset", 10, -30, 30, " deg") };
        for (auto* m : {&m1, &m2, &m3, &m4, &m5, &m6, &m7, &m8, &m9, &m10, &m11, &m12, &m13,
                        &m14, &m15, &m16, &m17, &m18, &m19, &m20, &m21, &m22, &m23, &m24, &m25}) {
            finishMod(*m);
            c.modules.push_back(*m);
        }
        categories.push_back(std::move(c));
    }
    // ---- Friends: dedicated list window (CategoryList.lua), not a module ----
    friendsOpen = false;
    friendsExpanded = true;
    friendsOptionsOpen = false;
    friends.clear();
    friendBuf[0] = '\0';
    friendsRecolor = true;
    friendsUse = true;
    friendsHue = 0.46f; friendsSat = 0.96f; friendsVal = 0.52f;
    bindingModule = nullptr;
    for (int i = 0; i < 256; ++i) bindCaptureSeed[i] = false;

    // Default layout: one aligned row of windows 16px under the floating
    // search bar (kRowTopY, also the main window's top line), evenly spaced to
    // the right of the main window (the reference spreads a single top row
    // across the screen).
    const float rowY = kRowTopY;
    float rowX = 256.0f; // main window (12) + kPanelW + 24 gap
    for (auto& c : categories) {
        if (c.isLegit)
            continue; // rendered by the standalone legit window (legitX/legitY)
        c.posX = rowX;
        c.posY = rowY;
        rowX += kPanelW + 16.0f;
    }
    // Friends stacks at the end of the row, like the category windows.
    friendsX = friendsDefX = rowX;
    friendsY = friendsDefY = rowY;

    for (auto& c : categories) {
        c.defX = c.posX;
        c.defY = c.posY;
    }
    ResetPositions();
}

