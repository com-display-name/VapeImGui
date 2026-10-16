// Module rows and their option widgets (Module/Toggle/Slider/Dropdown/
// Textbox): layout, rendering and interaction.
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

// ============================= Module row + options =============================
void VapeGui::RenderModuleRow(vape::Category& cat, vape::Module& mod, const ImVec4& accentRgb, float t, float y, float& used) {
    (void)cat; (void)y; (void)used;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Uniform width for every row (see RowWidth): enabled/disabled share the
    // same size so the outline never changes shade per row.
    rowFullBleed = true;

    float w = RowWidth(rowFullBleed);

    ImVec4 modAccent = AccentForIndex(accent, mod.index, t);
    float ah, as, av;
    AccentHSV(accent, mod.index, t, ah, as, av);

    std::string id = "##mod_" + mod.name + "_" + std::to_string(mod.index);
    ImGui::InvisibleButton(id.c_str(), ImVec2(w, kModuleH));
    bool hov = ImGui::IsItemHovered();
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    float dotsX0 = r1.x - 25.0f;
    bool inDots = ImGui::GetIO().MousePos.x >= dotsX0 && hov;

    // Keybind chip (Bind.lua, module variant): 20x20, white 8% bg, 4px
    // corner, right of the dots zone. Visible when bound, hovered, expanded
    // or while this module is capturing.
    bool capturing = (bindingModule == &mod);
    std::string bindName = mod.bindKey ? BindKeyName(mod.bindKey) : "";
    float chipW = 20.0f;
    if (!bindName.empty()) {
        ImVec2 bts = ImGui::CalcTextSize(bindName.c_str());
        chipW = std::max(bts.x + 10.0f, 20.0f);
    }
    bool showChip = !editMode && (mod.bindKey != 0 || hov || mod.expanded || capturing);
    ImVec2 chipMin(0, 0), chipMax(0, 0);
    bool chipHover = false;
    if (showChip) {
        chipMax = ImVec2(r1.x - 28.0f, r0.y + 30.0f);
        chipMin = ImVec2(chipMax.x - chipW, r0.y + 10.0f);
        ImVec2 mp = ImGui::GetIO().MousePos;
        chipHover = hov && mp.x >= chipMin.x && mp.x <= chipMax.x &&
                    mp.y >= chipMin.y && mp.y <= chipMax.y;
    }

    if (!editMode) {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            if (chipHover) {
                bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                if (capturing) {
                    mod.bindKey = 0; // second click removes the bind
                    bindingModule = nullptr;
                } else if (shift && mod.bindKey != 0) {
                    mod.bindHold = !mod.bindHold; // hold vs toggle
                } else {
                    // start capture; seed currently-held keys so an already
                    // held key is not instantly bound
                    bindingModule = &mod;
                    for (int i = 0; i < 52; ++i) {
                        int vk = BindableKeyAt(i);
                        if (vk > 0 && vk < 256)
                            bindCaptureSeed[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
                    }
                    bindCaptureSeed[VK_ESCAPE] = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
                }
            } else if (capturing) {
                // ignore row clicks elsewhere while capturing a key
            } else if (inDots) {
                mod.expanded = !mod.expanded;
            } else {
                mod.enabled = !mod.enabled;
                if (toggleNotifications)
                    Notify(mod.name.c_str(), mod.enabled ? "Enabled" : "Disabled", 1.5f, mod.enabled ? 0 : 1);
            }
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !capturing)
            mod.expanded = !mod.expanded;
    } else {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            mod.visible = !mod.visible;
    }

    // background
    ImVec4 rowBg;
    if (!editMode && mod.enabled) rowBg = modAccent;
    else if (hov || mod.expanded) rowBg = Lighten(MainColor(), 0.02f);
    else rowBg = MainColor();
    // options area is drawn separately; row itself:
    dl->AddRectFilled(r0, r1, ToU32(rowBg));

    // text color
    ImVec4 nameCol;
    if (!editMode && mod.enabled) nameCol = AccentText(ah, as, av);
    else if (hov || mod.expanded) nameCol = txt;
    else nameCol = dim;
    if (editMode && !mod.visible) nameCol = Darken(txt, 0.35f);

    // Vape pads module text with hidden chars (~12px). We use 14px offset.
    dl->AddText(ImVec2(r0.x + 14, r0.y + 11), ToU32(nameCol), mod.name.c_str());

    if (!editMode) {
        // "PRESS A KEY TO BIND" cover while this module is capturing.
        if (capturing) {
            dl->AddRectFilled(r0, r1, IM_COL32(8, 8, 8, 220));
            const char* ct = "PRESS A KEY TO BIND";
            ImVec2 cts = ImGui::CalcTextSize(ct);
            dl->AddText(ImVec2((r0.x + r1.x - cts.x) * 0.5f, (r0.y + r1.y - cts.y) * 0.5f),
                        ToU32(txt), ct);
        }
        // keybind chip (drawn over the cover so it stays clickable)
        if (showChip) {
            dl->AddRectFilled(chipMin, chipMax, IM_COL32(255, 255, 255, 20), 4.0f);
            ImVec4 chipCol = chipHover ? Darken(txt, 0.16f) : Darken(txt, 0.43f);
            ImVec2 cc((chipMin.x + chipMax.x) * 0.5f, (chipMin.y + chipMax.y) * 0.5f);
            if (capturing) {
                const char* dots = "...";
                ImVec2 dts = ImGui::CalcTextSize(dots);
                dl->AddText(ImVec2(cc.x - dts.x * 0.5f, cc.y - dts.y * 0.5f), ToU32(chipCol), dots);
            } else if (bindName.empty()) {
                DrawIcon("bind.png", ImVec2(cc.x - 5, cc.y - 5), ImVec2(10, 10), ToU32(chipCol));
            } else {
                ImVec2 bts = ImGui::CalcTextSize(bindName.c_str());
                dl->AddText(ImVec2(cc.x - bts.x * 0.5f, cc.y - bts.y * 0.5f),
                            ToU32(chipCol), bindName.c_str());
            }
            if (chipHover) {
                std::string tip = "Click to bind\nShift click to modify bind functionality\nBind functionality = ";
                tip += mod.bindHold ? "Enable while held" : "Toggle";
                ShowTooltip(tip.c_str());
            }
        }
        // dots affordance
        ImU32 dotsCol;
        if (mod.enabled) dotsCol = ToU32(nameCol);
        else if (hov || inDots || mod.expanded) dotsCol = ToU32(txt);
        else dotsCol = ToU32(Lighten(MainColor(), 0.37f));
        auto it = icons.find("settingdots.png");
        if (it != icons.end() && it->second.srv) {
            // reference: 3x16 glyph at (W-21, row+12)
            dl->AddImage((ImTextureID)it->second.srv, ImVec2(r1.x - 20, r0.y + 12), ImVec2(r1.x - 17, r0.y + 28),
                         ImVec2(0, 0), ImVec2(1, 1), dotsCol);
        } else {
            for (int i = 0; i < 3; ++i)
                dl->AddCircleFilled(ImVec2(r1.x - 18.5f, r0.y + 14 + i * 6), 1.5f, dotsCol);
        }
        // enabled divider
        if (mod.enabled)
            dl->AddRectFilled(ImVec2(r0.x, r1.y - 1), r1, IM_COL32(48, 48, 48, 122));
        if (!chipHover && !mod.name.empty() && hov && !mod.tooltip.empty())
            ShowTooltip(mod.tooltip.c_str());
    } else {
        // edit checkbox 8x8
        ImVec2 b0 = ImVec2(r1.x - 24, r0.y + 16);
        ImVec2 b1 = ImVec2(r1.x - 16, r0.y + 24);
        if (mod.visible)
            dl->AddRectFilled(b0, b1, ToU32(modAccent));
        else
            dl->AddRect(b0, b1, ToU32(Lighten(MainColor(), 0.37f)));
    }

    // options
    if (mod.expanded && !editMode) {
        ImVec4 optBg = Darken(MainColor(), 0.02f);
        for (auto& o : mod.options) {
            float oh = OptionHeight(o);
            float oy = ImGui::GetCursorPosY();
            (void)oy;
            switch (o.type) {
            case vape::OptionType::Toggle: RenderToggleOption(mod, o.toggle, modAccent, 0, oh); break;
            case vape::OptionType::Slider: RenderSliderOption(o.slider, modAccent, 0, oh); break;
            case vape::OptionType::TwoSlider: RenderTwoSliderOption(o.twoslider, modAccent, 0, oh); break;
            case vape::OptionType::Dropdown: RenderDropdownOption(o.dropdown, 0, oh); break;
            case vape::OptionType::TextBox: RenderTextBoxOption(o.textbox, 0, oh); break;
            }
            // paint option bg behind (row draws after? draw rect under using item rect)
            (void)optBg;
        }
    }
}

void VapeGui::RenderToggleOption(vape::Module& mod, vape::ToggleOption& opt, const ImVec4& accentRgb, float y, float& used) {
    (void)mod; (void)y;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = RowWidth(rowFullBleed);

    std::string id = "##tgl_" + opt.name + "_" + mod.name;
    ImGui::InvisibleButton(id.c_str(), ImVec2(w, kToggleH));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        opt.enabled = !opt.enabled;
    bool hov = ImGui::IsItemHovered();
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    dl->AddRectFilled(r0, r1, ToU32(Darken(MainColor(), 0.02f)));
    dl->AddText(ImVec2(r0.x + 10, r0.y + 7), ToU32(hov || opt.enabled ? txt : dim), opt.name.c_str());

    // switch: holder 22x12 rounded, knob 8x8
    ImVec2 h0 = ImVec2(r1.x - 30, r0.y + 9);
    ImVec2 h1 = ImVec2(r1.x - 8, r0.y + 21);
    ImVec4 holderCol = opt.enabled ? accentRgb
                      : (hov ? Lighten(MainColor(), 0.37f) : Lighten(MainColor(), 0.14f));
    dl->AddRectFilled(h0, h1, ToU32(holderCol), 6.0f);
    float kx = opt.enabled ? (h1.x - 10) : (h0.x + 2);
    dl->AddRectFilled(ImVec2(kx, h0.y + 2), ImVec2(kx + 8, h0.y + 10), ToU32(MainColor()), 4.0f);
    (void)used;
}

void VapeGui::RenderSliderOption(vape::SliderOption& opt, const ImVec4& accentRgb, float y, float& used) {
    (void)y;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = RowWidth(rowFullBleed);

    std::string id = "##sld_" + opt.name;
    ImGui::InvisibleButton(id.c_str(), ImVec2(w, kSliderH));
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    bool hov = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    dl->AddRectFilled(r0, r1, ToU32(Darken(MainColor(), 0.02f)));

    dl->AddText(ImVec2(r0.x + 10, r0.y + 4), ToU32(dim), opt.name.c_str());
    char valBuf[48];
    if (opt.decimal >= 10.0f)
        snprintf(valBuf, sizeof(valBuf), "%g %s", opt.value, opt.suffix.c_str());
    else
        snprintf(valBuf, sizeof(valBuf), "%d %s", (int)opt.value, opt.suffix.c_str());
    ImVec2 vsz = ImGui::CalcTextSize(valBuf);
    dl->AddText(ImVec2(r1.x - 10 - vsz.x, r0.y + 4), ToU32(dim), valBuf);

    // track
    ImVec2 t0 = ImVec2(r0.x + 10, r0.y + 37);
    ImVec2 t1 = ImVec2(r1.x - 10, r0.y + 39);
    dl->AddRectFilled(t0, t1, ToU32(Lighten(MainColor(), 0.034f)));
    float span = opt.maxV - opt.minV;
    float frac = span > 0 ? (opt.value - opt.minV) / span : 0;
    frac = std::clamp(frac, 0.04f, 0.96f);
    float fx = t0.x + (t1.x - t0.x) * frac;
    dl->AddRectFilled(t0, ImVec2(fx, t1.y), ToU32(accentRgb));
    // knob 14x14 (16 on hover), centered on fill end
    float kr = (hov || active) ? 8.0f : 7.0f;
    float ky = (t0.y + t1.y) * 0.5f;
    // knob holder ring (matches vape knobholder bg)
    dl->AddCircleFilled(ImVec2(fx, ky), kr + 5, ToU32(Darken(MainColor(), 0.02f)));
    dl->AddCircleFilled(ImVec2(fx, ky), kr, ToU32(accentRgb));

    // drag on lower area (y > 20 within row), like lua (20*scale)
    ImVec2 mp = ImGui::GetIO().MousePos;
    if (active && mp.y > r0.y + 20) {
        float nt = std::clamp((mp.x - t0.x) / (t1.x - t0.x), 0.0f, 1.0f);
        float raw = opt.minV + span * nt;
        float nv = floorf(raw * opt.decimal) / opt.decimal;
        nv = std::clamp(nv, opt.minV, opt.maxV);
        if (nv != opt.value)
            opt.value = nv;
    }
    (void)used;
}

// TwoSlider.lua: one track with a min and a max handle. Clicking a value
// label swaps it for a text box; Enter applies, losing focus discards (the
// reference only applies on FocusLost with enter == true).
void VapeGui::RenderTwoSliderOption(vape::TwoSliderOption& opt, const ImVec4& accentRgb, float y, float& used) {
    (void)y;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = RowWidth(rowFullBleed);

    ImVec2 r0 = ImGui::GetCursorScreenPos();
    ImVec2 r1 = ImVec2(r0.x + w, r0.y + kSliderH);
    ImVec2 mp = ImGui::GetIO().MousePos;

    // Value label boxes (reference: min at w-125, max at w-69, both 60 wide).
    float minBoxX = r1.x - 125.0f, maxBoxX = r1.x - 69.0f;
    auto labelHit = [&](float bx) {
        return ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mp.x >= bx && mp.x < bx + 60.0f &&
               mp.y >= r0.y + 2.0f && mp.y < r0.y + 20.0f;
    };
    if (opt.editSide == 0) {
        if (labelHit(minBoxX)) {
            opt.editSide = 1;
            snprintf(opt.editText, sizeof(opt.editText), "%g", opt.valueMin);
        } else if (labelHit(maxBoxX)) {
            opt.editSide = 2;
            snprintf(opt.editText, sizeof(opt.editText), "%g", opt.valueMax);
        }
    }

    // Edit box submitted BEFORE the row button so it owns the hover (an
    // InvisibleButton first would claim it, see RenderTextBoxOption).
    if (opt.editSide != 0) {
        ImGui::SetCursorScreenPos(ImVec2(opt.editSide == 1 ? minBoxX : maxBoxX, r0.y + 2));
        ImGui::PushItemWidth(60);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ToU32(txt));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 4));
        ImGui::SetKeyboardFocusHere();
        std::string eid = "##tsin_" + opt.name + (opt.editSide == 1 ? "_min" : "_max");
        if (ImGui::InputText(eid.c_str(), opt.editText, IM_ARRAYSIZE(opt.editText),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            float nv = 0.0f;
            if (sscanf(opt.editText, "%f", &nv) == 1 && std::isfinite(nv)) {
                nv = std::clamp(floorf(nv * opt.decimal) / opt.decimal, opt.minV, opt.maxV);
                if (opt.editSide == 1) opt.valueMin = nv;
                else opt.valueMax = nv;
            }
            opt.editSide = 0;
        } else if (ImGui::IsItemDeactivated()) {
            opt.editSide = 0; // focus lost without Enter discards
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopItemWidth();
    }

    std::string id = "##tsl_" + opt.name;
    ImGui::SetCursorScreenPos(r0);
    ImGui::InvisibleButton(id.c_str(), ImVec2(w, kSliderH));
    bool hov = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    dl->AddRectFilled(r0, r1, ToU32(Darken(MainColor(), 0.02f)));
    dl->AddText(ImVec2(r0.x + 10, r0.y + 4), ToU32(dim), opt.name.c_str());

    // min -> max labels with the range arrow between them (reference:
    // rangeindicator.png 12x6, Light(Main, 0.14)).
    auto fmtLabel = [&](float v, float textRightX) {
        char b[16];
        snprintf(b, sizeof(b), "%g", v);
        ImVec2 sz = ImGui::CalcTextSize(b);
        dl->AddText(ImVec2(textRightX - sz.x, r0.y + 4), ToU32(dim), b);
    };
    if (opt.editSide != 1) fmtLabel(opt.valueMin, r1.x - 65.0f);
    if (opt.editSide != 2) fmtLabel(opt.valueMax, r1.x - 10.0f);
    ImVec2 ap = ImVec2(r1.x - 57.0f, r0.y + 7.0f);
    dl->AddTriangleFilled(ap, ImVec2(ap.x, ap.y + 6.0f), ImVec2(ap.x + 12.0f, ap.y + 3.0f),
                          ToU32(Lighten(MainColor(), 0.14f)));

    // Track with the accent fill spanning the two handles. Display positions
    // clamp to [0.04, 0.96] like the reference so the grips stay on the track.
    ImVec2 t0 = ImVec2(r0.x + 10, r0.y + 37);
    ImVec2 t1 = ImVec2(r1.x - 10, r0.y + 39);
    dl->AddRectFilled(t0, t1, ToU32(Lighten(MainColor(), 0.034f)));
    float span = opt.maxV - opt.minV;
    auto fracOf = [&](float v) {
        float f = span > 0.0f ? (v - opt.minV) / span : 0.0f;
        return std::clamp(f, 0.04f, 0.96f);
    };
    float fMin = fracOf(opt.valueMin), fMax = fracOf(opt.valueMax);
    float xa = t0.x + (t1.x - t0.x) * fMin;
    float xb = t0.x + (t1.x - t0.x) * std::max(fMin, fMax); // inverted range = no fill, like the lua
    dl->AddRectFilled(ImVec2(xa, t0.y), ImVec2(xb, t1.y), ToU32(accentRgb));

    // Grips at both fill ends (reference: range.png, a 9x16 accent bar on a
    // 16x4 holder; the bar grows to 11x18 on hover).
    float ky = (t0.y + t1.y) * 0.5f;
    auto grip = [&](float gx, bool big) {
        dl->AddRectFilled(ImVec2(gx - 8, ky - 2), ImVec2(gx + 8, ky + 2), ToU32(Darken(MainColor(), 0.02f)));
        float gh = big ? 8.0f : 7.0f, gw = big ? 2.5f : 2.0f;
        dl->AddRectFilled(ImVec2(gx - gw, ky - gh), ImVec2(gx + gw, ky + gh), ToU32(accentRgb), 2.0f);
    };
    grip(xa, hov && std::fabs(mp.x - xa) < 10.0f);
    grip(xb, hov && std::fabs(mp.x - xb) < 10.0f);

    // Drag: a press in the lower half picks the handle the lua picks (max
    // when the press is at/right of the max grip minus 10); it then follows
    // the mouse until release. The handles may cross, like the reference.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        twoSliderDrag = (mp.x - xb) > -10.0f ? 2 : 1;
    if (active && twoSliderDrag != 0 && mp.y > r0.y + 20) {
        float nt = std::clamp((mp.x - t0.x) / (t1.x - t0.x), 0.0f, 1.0f);
        float nv = std::clamp(floorf((opt.minV + span * nt) * opt.decimal) / opt.decimal, opt.minV, opt.maxV);
        if (twoSliderDrag == 2) opt.valueMax = nv;
        else opt.valueMin = nv;
    }
    (void)used;
}

void VapeGui::RenderDropdownOption(vape::DropdownOption& opt, float y, float& used) {
    (void)y;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = RowWidth(rowFullBleed);

    std::string id = "##dd_" + opt.name;
    ImGui::InvisibleButton(id.c_str(), ImVec2(w, 30));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        opt.open = !opt.open;
    bool hov = ImGui::IsItemHovered();
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    dl->AddRectFilled(r0, r1, ToU32(Darken(MainColor(), 0.02f)));
    dl->AddText(ImVec2(r0.x + 10, r0.y + 7), ToU32(hov ? txt : dim), opt.name.c_str());
    const char* cur = opt.selected >= 0 && opt.selected < (int)opt.items.size() ? opt.items[opt.selected].c_str() : "";
    ImVec2 csz = ImGui::CalcTextSize(cur);
    dl->AddText(ImVec2(r1.x - 26 - csz.x, r0.y + 7), ToU32(dim), cur);
    // arrow
    ImVec2 ap = ImVec2(r1.x - 18, r0.y + 13);
    ImU32 acol = ToU32(Lighten(MainColor(), 0.37f));
    if (opt.open)
        dl->AddTriangleFilled(ImVec2(ap.x, ap.y), ImVec2(ap.x + 9, ap.y), ImVec2(ap.x + 4.5f, ap.y + 4.5f), acol);
    else
        dl->AddTriangleFilled(ImVec2(ap.x, ap.y + 4.5f), ImVec2(ap.x + 9, ap.y + 4.5f), ImVec2(ap.x + 4.5f, ap.y), acol);

    if (opt.open) {
        for (int i = 0; i < (int)opt.items.size(); ++i) {
            std::string iid = "##ddi_" + opt.name + "_" + std::to_string(i);
            ImGui::InvisibleButton(iid.c_str(), ImVec2(w, 26));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                opt.selected = i;
                opt.open = false;
            }
            bool ih = ImGui::IsItemHovered();
            ImVec2 i0 = ImGui::GetItemRectMin(), i1 = ImGui::GetItemRectMax();
            if (ih) dl->AddRectFilled(i0, i1, ToU32(Lighten(MainColor(), 0.06f)));
            dl->AddText(ImVec2(i0.x + 22, i0.y + 5),
                        ToU32(i == opt.selected ? txt : dim), opt.items[i].c_str());
        }
    }
    (void)used;
}

void VapeGui::RenderTextBoxOption(vape::TextBoxOption& opt, float y, float& used) {
    (void)y;
    using namespace vape;
    ImVec4 txt = TextColor();
    ImVec4 dim = Darken(txt, 0.16f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = RowWidth(rowFullBleed);

    ImVec2 r0 = ImGui::GetCursorScreenPos();
    // NOTE: a plain Dummy, not an InvisibleButton. An InvisibleButton over the
    // row claims hover first, so the InputText below could never be hovered or
    // clicked - that is why the profile / friends / keyword fields were dead.
    ImGui::Dummy(ImVec2(w, 40));
    ImVec2 r1 = ImVec2(r0.x + w, r0.y + 40);
    dl->AddRectFilled(r0, r1, ToU32(Darken(MainColor(), 0.02f)));
    dl->AddText(ImVec2(r0.x + 10, r0.y + 12), ToU32(dim), opt.name.c_str());

    ImGui::SetCursorScreenPos(ImVec2(r1.x - 122, r0.y + 9));
    ImGui::PushItemWidth(112);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ToU32(Lighten(MainColor(), 0.06f)));
    ImGui::PushStyleColor(ImGuiCol_Text, ToU32(txt));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::InputText(("##tbin_" + opt.name).c_str(), opt.text, IM_ARRAYSIZE(opt.text));
    ImGui::PopItemWidth(); // was leaking into every widget submitted afterwards
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    (void)used;
}
