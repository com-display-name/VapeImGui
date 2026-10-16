// Vape V4 ClickGUI - core: the global instance, shared widgets (tooltip,
// icons, notification stack push), keybind polling and the per-frame
// Render() orchestrator. Every panel lives in its own vape_*.cpp sibling;
// shared free helpers live in vape_internal.h.
#include "vape_gui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include "vape_internal.h"

VapeGui g_vape;

void VapeGui::SetIcon(const std::string& name, ID3D11ShaderResourceView* srv, int w, int h) {
    icons[name] = VapeIcon{srv, w, h};
}

void VapeGui::Notify(const std::string& title, const std::string& text, float duration, int kind) {
    if (!toggleNotifications && kind == 0)
        return; // toggle notifications gate (alerts still show)
    vape::Notification n;
    n.title = title; n.text = text;
    n.created = VapeTime();
    n.duration = duration; n.kind = kind;
    notifications.push_back(n);
    if (notifications.size() > 5)
        notifications.erase(notifications.begin());
}

float VapeGui::OptionHeight(const vape::Option& o) const {
    switch (o.type) {
    case vape::OptionType::Toggle: return vape::kToggleH;
    case vape::OptionType::Slider: return vape::kSliderH;
    case vape::OptionType::TwoSlider: return vape::kSliderH; // TwoSlider.lua: same 50px row
    case vape::OptionType::Dropdown: return 30.0f + (o.dropdown.open ? (float)o.dropdown.items.size() * 26.0f : 0.0f);
    case vape::OptionType::TextBox: return 40.0f;
    }
    return 30.0f;
}

float VapeGui::ModuleHeight(const vape::Module& m) const {
    float h = kModuleH;
    if (m.expanded) {
        for (auto& o : m.options)
            h += OptionHeight(o);
    }
    return h;
}

void VapeGui::ShowTooltip(const char* text) {
    if (!text || !text[0])
        return;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4)); // frame +6x/+4y, text centred
    if (ImGui::BeginTooltip()) {
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar();
}

void VapeGui::DrawIcon(const char* name, ImVec2 pos, ImVec2 size, ImU32 tint) {
    auto it = icons.find(name);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (it != icons.end() && it->second.srv) {
        dl->AddImage((ImTextureID)it->second.srv, pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     ImVec2(0, 0), ImVec2(1, 1), tint);
    } else {
        // Fallback glyph so layout is visible when assets are missing.
        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), tint & 0x55FFFFFF, 2.0f);
    }
}

void VapeGui::ResetPositions() {
    mainX = 12.0f;
    mainY = vape::kRowTopY; // top-aligned with the category row
    for (auto& c : categories) {
        c.posX = c.defX;
        c.posY = c.defY;
    }
    friendsX = friendsDefX; // friends sits at the row's end (InitDemoData)
    friendsY = friendsDefY;
    // The search bar re-centres at the top on its next frame, same reason the
    // legit window keeps a <0 sentinel: the display size isn't known yet.
    searchX = searchY = -1.0f;
    searchDrag = false;
}
// ============================= Keybinds (Bind.lua) =============================
// Capture assigns the first freshly-pressed listed key (polled globally so it
// works while the game has focus); triggers toggle/hold bound modules.
void VapeGui::PollBinds() {
    if (bindingModule) {
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0 && !bindCaptureSeed[VK_ESCAPE])
            bindingModule = nullptr; // Escape cancels, keeps the old bind
        else {
            for (int i = 0; i < 52; ++i) {
                int vk = BindableKeyAt(i);
                if (vk <= 0 || vk >= 256) continue;
                if ((GetAsyncKeyState(vk) & 0x8000) != 0 && !bindCaptureSeed[vk]) {
                    bindingModule->bindKey = vk;
                    bindingModule->bindHold = false;
                    bindingModule->bindWasDown = true; // don't instantly trigger
                    Notify(bindingModule->name.c_str(),
                           ("Bound to " + BindKeyName(vk)).c_str(), 1.5f);
                    bindingModule = nullptr;
                    break;
                }
            }
        }
    }
    if (bindingModule || ImGui::GetIO().WantTextInput)
        return; // no triggers while capturing or typing
    for (auto& c : categories) {
        for (auto& m : c.modules) {
            if (m.bindKey <= 0) continue;
            bool down = (GetAsyncKeyState(m.bindKey) & 0x8000) != 0;
            if (m.bindHold) {
                if (m.enabled != down) {
                    m.enabled = down;
                    if (toggleNotifications)
                        Notify(m.name.c_str(), down ? "Enabled" : "Disabled", 1.5f, down ? 0 : 1);
                }
            } else if (down && !m.bindWasDown) {
                m.enabled = !m.enabled;
                if (toggleNotifications)
                    Notify(m.name.c_str(), m.enabled ? "Enabled" : "Disabled", 1.5f, m.enabled ? 0 : 1);
            }
            m.bindWasDown = down;
        }
    }
}
// ============================= FX overlay =============================
void VapeGui::Render() {
    float t = (float)ImGui::GetTime();
    ImVec4 accentRgb = AccentForIndex(accent, 0, t);

    hitRects.clear();

    PollBinds(); // keybind capture + triggers (runs even with GUI hidden)

    RenderTextGui(accentRgb);
    RenderNotifications();

    if (!mainOpen)
        return;
    // Legit Modules GUI swaps out the whole main GUI (SearchBar.lua: clicking
    // the legit switch hides the clickgui and shows the standalone window;
    // its close button brings the main GUI back).
    vape::Category* legitCat = FindCategory("Legit");
    if (legitCat && legitCat->open) {
        RenderLegit(*legitCat);
        return;
    }
    legitSettings = nullptr; // the options pane dies with the window
    RenderMain();
    RenderSearch(); // floating top-centre bar (drawn over the main window)
    for (auto& c : categories) {
        if (c.isLegit) continue; // rendered by RenderLegit instead
        if (c.open)
            RenderCategory(c);
    }
    if (friendsOpen)
        RenderFriends();
    if (settingsOpen)
        RenderSettings();
}
