// VapeUI overlay host (Dear ImGui + DirectX11): a single borderless
// fullscreen window per selected monitor. Panels float inside it; empty areas
// are click-through via exact panel-rect hit-testing (WM_NCHITTEST). The
// window never steals keyboard focus (WS_EX_NOACTIVATE) except briefly while
// typing in a text field. RSHIFT toggles the panels, END quits.
//
// File layout: overlay state + WndProc, monitor selection, icon loading,
// main-loop helpers, D3D11 device, then wWinMain (entry point) last.
#include <Windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>

#include <cstdio>
#include <string>
#include <vector>

#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/imgui_internal.h"
#include "../third_party/imgui/imgui_impl_win32.h"
#include "../third_party/imgui/imgui_impl_dx11.h"
#include "texture.h"
#include "vape_gui.h"

// --- DirectX11 state ---
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static bool g_opaque = false; // --opaque: solid background (debug)
static bool g_keyboardCapture = false;
static HWND g_focusOwner = nullptr; // window that had the keyboard before us
// Idle throttle: set by the WndProc whenever a message can change what is on
// screen; consumed by the main loop, which otherwise skips rendering entirely.
static bool g_uiInput = false;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Overlay window message routing ---
// WM_NCHITTEST owns the click-through behaviour; everything else sets the
// idle-throttle input flag (g_uiInput).
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    // Idle-throttle input flag: every message except the raw hit-test can
    // change what is on screen (mouse over a panel, keys while focused,
    // size/position changes). WM_NCHITTEST is excluded - it fires for ALL
    // mouse movement across the fullscreen window, including empty
    // click-through areas where nothing of ours changes.
    if (msg != WM_NCHITTEST)
        g_uiInput = true;
    switch (msg) {
    case WM_NCHITTEST: {
        // Click-through everywhere except exact panel rects (rebuilt each
        // frame; never derived from hover state, which can go stale).
        bool over = false;
        if (g_opaque) {
            over = true;
        } else {
            int sx = (int)(short)LOWORD(lParam); // sign-extended for negative monitor origins
            int sy = (int)(short)HIWORD(lParam);
            POINT pt = {sx, sy};
            ScreenToClient(hWnd, &pt);
            for (const HitRect& r : g_vape.hitRects) {
                if ((float)pt.x >= r.x0 && (float)pt.x < r.x1 &&
                    (float)pt.y >= r.y0 && (float)pt.y < r.y1) {
                    over = true;
                    break;
                }
            }
        }
        // Entering/leaving a panel flips hover state (highlights, tooltips),
        // which needs one redraw even though the hit-test itself carries no
        // content change.
        static bool lastOver = false;
        if (over != lastOver) {
            lastOver = over;
            g_uiInput = true;
        }
        return over ? HTCLIENT : HTTRANSPARENT;
    }
    case WM_PAINT:
        // Validate without painting: the layered window keeps whatever we
        // last presented; g_uiInput already schedules a redraw.
        ValidateRect(hWnd, nullptr);
        return 0;
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                                        DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// --- Monitors ---
struct OverlayMonitor {
    std::wstring device; // e.g. L"\\\\.\\DISPLAY1"
    std::string label;   // e.g. "#1 DISPLAY1 - 1920x1080 (Primary)"
    RECT rect = {0, 0, 0, 0};
    RECT work = {0, 0, 0, 0}; // work area (excludes taskbar/appbars)
    bool primary = false;
};
static std::vector<OverlayMonitor> g_monitors;
static bool g_insetMode = false; // "Fullscreen" toggle: 32px top bar + taskbar left uncovered

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM) {
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMon, &mi))
        return TRUE;
    OverlayMonitor m;
    m.device = mi.szDevice;
    m.rect = mi.rcMonitor;
    m.work = mi.rcWork;
    m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    char narrow[64] = {0};
    WideCharToMultiByte(CP_UTF8, 0, mi.szDevice, -1, narrow, (int)sizeof(narrow), nullptr, nullptr);
    const char* shortName = narrow;
    if (strncmp(shortName, "\\\\.\\", 4) == 0)
        shortName += 4;
    int w = mi.rcMonitor.right - mi.rcMonitor.left;
    int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    char label[128];
    snprintf(label, sizeof(label), "#%d %s - %dx%d%s",
             (int)g_monitors.size() + 1, shortName, w, h, m.primary ? " (Primary)" : "");
    m.label = label;
    g_monitors.push_back(m);
    return TRUE;
}

static std::string ExeDir() {
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string s = path;
    size_t p = s.find_last_of("\\/");
    return (p == std::string::npos) ? "." : s.substr(0, p);
}

static void SaveMonitorChoice(const std::wstring& device) {
    char narrow[64] = {0};
    WideCharToMultiByte(CP_UTF8, 0, device.c_str(), -1, narrow, (int)sizeof(narrow), nullptr, nullptr);
    std::string cfg = ExeDir() + "\\display.cfg";
    FILE* f = nullptr;
    if (fopen_s(&f, cfg.c_str(), "w") == 0 && f) {
        fprintf(f, "%s\n", narrow);
        fclose(f);
    }
}

static std::string LoadMonitorChoice() {
    std::string cfg = ExeDir() + "\\display.cfg";
    FILE* f = nullptr;
    if (fopen_s(&f, cfg.c_str(), "r") != 0 || !f)
        return "";
    char line[128] = {0};
    std::string out;
    if (fgets(line, (int)sizeof(line), f)) {
        out = line;
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' '))
            out.pop_back();
    }
    fclose(f);
    return out;
}

static std::string ToLower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// Resolve --monitor=N (1-based) or --monitor=NAME. Returns -1 if not given/matched.
static int ParseMonitorArg(const char* cmdLine) {
    std::string cmd = cmdLine ? cmdLine : "";
    const char* key = "--monitor=";
    size_t pos = ToLower(cmd).find(key);
    if (pos == std::string::npos)
        return -1;
    std::string val = cmd.substr(pos + strlen(key));
    size_t end = val.find(' ');
    if (end != std::string::npos)
        val = val.substr(0, end);
    while (!val.empty() && (val.front() == '"' || val.front() == '\'')) val.erase(val.begin());
    while (!val.empty() && (val.back() == '"' || val.back() == '\'')) val.pop_back();
    if (val.empty())
        return -1;
    bool numeric = true;
    for (auto c : val)
        if (!isdigit((unsigned char)c)) { numeric = false; break; }
    if (numeric) {
        int n = atoi(val.c_str());
        if (n >= 1 && n <= (int)g_monitors.size())
            return n - 1;
        return -1;
    }
    std::string needle = ToLower(val);
    for (int i = 0; i < (int)g_monitors.size(); ++i) {
        if (ToLower(g_monitors[i].label).find(needle) != std::string::npos)
            return i;
    }
    return -1;
}

static void ApplyWindowRect() {
    if (!g_hWnd || g_monitors.empty())
        return;
    int idx = g_vape.monitorIndex;
    if (idx < 0 || idx >= (int)g_monitors.size())
        idx = 0;
    const OverlayMonitor& m = g_monitors[idx];
    RECT r = m.rect;
    if (g_insetMode) {
        // UDim2.new(1,0,1,-(32 + TaskbarSize)) offset by (0,0,0,32):
        // 32px top bar uncovered, bottom clamped to the work area (taskbar).
        r.top = m.rect.top + 32;
        r.bottom = m.work.bottom;
        if (r.bottom < r.top + 100)
            r.bottom = r.top + 100;
    }
    SetWindowPos(g_hWnd, g_vape.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 r.left, r.top, r.right - r.left, r.bottom - r.top,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
    // Swapchain buffers are resized in WM_SIZE.
}

static void MoveToMonitor(int idx) {
    if (idx < 0 || idx >= (int)g_monitors.size() || !g_hWnd)
        return;
    g_vape.monitorIndex = idx;
    ApplyWindowRect();
    SaveMonitorChoice(g_monitors[idx].device);
}

// --- Icons ---
static void LoadIcons() {
    const char* files[] = {
        "combat.png", "blatant.png", "render.png", "utility.png", "world.png",
        "inventory.png", "friends.png", "profiles.png", "settings.png", "search.png",
        "downexpand.png", "expandarrow.png", "settingdots.png", "discord.png",
        "editlarge.png", "vape.png", "vapelogo.png", "vapelogomini.png", "v4mini.png",
        "v4.png", "close.png", "noti_info.png", "noti_alert.png", "noti_warning.png",
        "textgui.png", "add.png", "pin.png", "backmini.png", "closetiny.png",
        "bind.png", "legit_switch.png", "legit_mode_icon.png", "min.png",
        "back.png", "overlaydots.png",
    };
    int loaded = 0;
    for (auto f : files) {
        std::string path = FindIconPath(f);
        if (path.empty())
            continue;
        ID3D11ShaderResourceView* srv = nullptr;
        int w = 0, h = 0;
        if (LoadTextureFromFile(path, g_pd3dDevice, &srv, &w, &h)) {
            g_vape.SetIcon(f, srv, w, h);
            loaded++;
        }
    }
    printf("[VapeUI] loaded %d/%d icons\n", loaded, (int)(sizeof(files) / sizeof(files[0])));
    if (loaded == 0)
        printf("[VapeUI] WARNING: no icons found - check assets/icons next to the vcxproj\n");
}

// Poll-only key edges for the idle throttle: while the game owns the keyboard,
// key presses never enter our queue, so bound keys, RSHIFT/END, and the mouse
// buttons must be edge-polled every loop pass (the sweep is what wakes PollBinds
// after the throttle skips idle frames).
static bool PollKeyEdges() {
    static const int keys[] = {
        VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_SHIFT, VK_CONTROL, VK_MENU,
        VK_ESCAPE, VK_SPACE, VK_RSHIFT, VK_END,
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
        VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12,
    };
    static bool prev[sizeof(keys) / sizeof(keys[0])] = {};
    bool edge = false;
    for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); ++i) {
        bool down = (GetAsyncKeyState(keys[i]) & 0x8000) != 0;
        if (down != prev[i]) {
            prev[i] = down;
            edge = true;
        }
    }
    return edge;
}

// WS_EX_NOACTIVATE bookkeeping: the overlay only takes the keyboard (and
// foreground) while a text field is focused, and hands both straight back to
// the game afterwards - otherwise a keystroke typed into our field would leave
// us foreground and the game would stop receiving keys entirely.
static void SetTypingCapture(bool wantText) {
    if (wantText == g_keyboardCapture)
        return;
    g_keyboardCapture = wantText;
    LONG_PTR ex = GetWindowLongPtr(g_hWnd, GWL_EXSTYLE);
    if (wantText)
        ex &= ~WS_EX_NOACTIVATE;
    else
        ex |= WS_EX_NOACTIVATE;
    SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, ex);
    SetWindowPos(g_hWnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (wantText) {
        // Take the keyboard even though another process owns the
        // foreground: attach to its input queue first, otherwise
        // SetForegroundWindow is silently denied and every keystroke
        // still goes to the game - the field looks dead.
        HWND prev = GetForegroundWindow();
        if (prev && prev != g_hWnd)
            g_focusOwner = prev;
        DWORD fgTid = prev ? GetWindowThreadProcessId(prev, nullptr) : 0;
        DWORD myTid = GetCurrentThreadId();
        BOOL attached = (fgTid && fgTid != myTid)
                            ? AttachThreadInput(myTid, fgTid, TRUE)
                            : FALSE;
        BringWindowToTop(g_hWnd);
        SetForegroundWindow(g_hWnd);
        SetFocus(g_hWnd);
        if (attached)
            AttachThreadInput(myTid, fgTid, FALSE);
    } else if (g_focusOwner) {
        // Hand focus back the moment typing ends, otherwise we stay
        // foreground and the game keeps ignoring the keyboard.
        HWND owner = g_focusOwner;
        g_focusOwner = nullptr;
        if (IsWindow(owner) && owner != g_hWnd)
            SetForegroundWindow(owner);
    }
}

// --- D3D11 device & render target (created in wWinMain, resized in WM_SIZE) ---
static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                                createDeviceFlags, featureLevelArray, 2,
                                                D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                                &g_pd3dDevice, &featureLevel,
                                                &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                            createDeviceFlags, featureLevelArray, 2,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                            &g_pd3dDevice, &featureLevel,
                                            &g_pd3dDeviceContext);
    }
    if (FAILED(res))
        return false;
    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    // DPI awareness must be enabled BEFORE the monitor enumeration below:
    // a DPI-unaware process gets virtualized (scaled) rects, and once we
    // become aware the fullscreen window ends up undersized/offset on any
    // display above 100% scaling.
    ImGui_ImplWin32_EnableDpiAwareness();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);
    if (g_monitors.empty()) {
        return 1;
    }

    const char* cmd = GetCommandLineA();
    std::string lowCmd = ToLower(cmd ? cmd : "");
    if (lowCmd.find("--opaque") != std::string::npos) {
        g_opaque = true;
        printf("[VapeUI] --opaque debug mode: solid background, mouse captured\n");
    }
    // Single instance. Mutex keeps the pre-rename name on purpose so an old
    // VapeImGui.exe still running cannot overlap a new VapeUI.exe.
    // (--list-monitors above still works while the app runs.)
    HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, L"VapeImGuiOverlaySingleton");
    if (!instanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (instanceMutex)
            CloseHandle(instanceMutex);
        MessageBoxW(nullptr, L"VapeUI overlay is already running.", L"VapeUI",
                    MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return 0;
    }

    // Pick monitor: CLI > saved display.cfg > primary > first.
    int monitorIdx = -1;
    for (int i = 0; i < (int)g_monitors.size(); ++i)
        if (g_monitors[i].primary) { monitorIdx = i; break; }
    if (monitorIdx < 0) monitorIdx = 0;
    std::string saved = LoadMonitorChoice();
    if (!saved.empty()) {
        for (int i = 0; i < (int)g_monitors.size(); ++i) {
            char narrow[64] = {0};
            WideCharToMultiByte(CP_UTF8, 0, g_monitors[i].device.c_str(), -1,
                                narrow, (int)sizeof(narrow), nullptr, nullptr);
            if (saved == narrow) { monitorIdx = i; break; }
        }
    }
    int cliIdx = ParseMonitorArg(cmd);
    if (cliIdx >= 0)
        monitorIdx = cliIdx;

    for (auto& m : g_monitors)
        g_vape.monitorNames.push_back(m.label);
    g_vape.monitorIndex = monitorIdx;
    printf("[VapeUI] using %s\n", g_monitors[monitorIdx].label.c_str());

    // Borderless fullscreen overlay: no titlebar, no border, not user-resizable.
    WNDCLASSEXW wc = {sizeof(wc), 0, WndProc, 0L, 0L,
                      GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                      L"VapeUIOverlay", nullptr};
    ::RegisterClassExW(&wc);
    const RECT& mr = g_monitors[monitorIdx].rect;
    DWORD exStyle = WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED;
    g_hWnd = ::CreateWindowExW(exStyle,
                               wc.lpszClassName, L"Vape V4",
                               WS_POPUP, mr.left, mr.top,
                               mr.right - mr.left, mr.bottom - mr.top,
                               nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hWnd) {
        printf("[VapeUI] CreateWindowEx failed (%lu)\n", GetLastError());
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    if (!g_opaque) {
        // Explicit colorkey: pure-black pixels are see-through (our clear
        // color; every UI color is non-black). Required - without attributes
        // a layered window may composite as invisible or opaque black.
        SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        MARGINS margins = {-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(g_hWnd, &margins);
    }

    if (!CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D();
        ::DestroyWindow(g_hWnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // we manage positions ourselves (like vape profiles)

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.CircleTessellationMaxError = 0.1f;
    style.ScrollbarRounding = 3.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    // Font: Arial (vape uses Arial) with ImGui default fallback.
    {
        const char* arial = "C:\\Windows\\Fonts\\arial.ttf";
        if (GetFileAttributesA(arial) != INVALID_FILE_ATTRIBUTES)
            io.Fonts->AddFontFromFileTTF(arial, 15.0f);
        else
            io.Fonts->AddFontDefault();
    }

    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    LoadIcons();
    g_vape.InitDemoData();
    g_vape.Notify("Vape", "Overlay ready - RSHIFT toggles GUI, END quits", 3.0f);

    g_vape.onMonitorChange = [](int idx) { MoveToMonitor(idx); };
    g_vape.onTopmostChange = [](bool top) {
        if (g_hWnd)
            SetWindowPos(g_hWnd, top ? HWND_TOPMOST : HWND_NOTOPMOST,
                         0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    };
    g_vape.showConsole = false;
    g_vape.fullscreen = true; // startup covers the whole monitor (inset off)
    g_vape.onFullscreenChange = [](bool on) {
        g_insetMode = !on;
        ApplyWindowRect();
    };
    g_vape.onScreenproofChange = [](bool on) {
        if (!g_hWnd)
            return;
        // 0x11 = WDA_EXCLUDEFROMCAPTURE (Win10 2004+): the window is omitted
        // from screen capture/sharing but stays visible on the monitor.
        // 0 = WDA_NONE. On pre-2004 builds the value degrades to WDA_MONITOR
        // (blacked out of capture; still visible to the user).
        if (!SetWindowDisplayAffinity(g_hWnd, on ? 0x11 : 0x0))
            printf("[VapeUI] SetWindowDisplayAffinity failed (%lu)\n", GetLastError());
    };

    const ImVec4 clearColor = g_opaque ? ImVec4(0.039f, 0.039f, 0.043f, 1.0f)
                                       : ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // pure black = colorkey
    bool done = false;
    bool rshiftDown = false;
    bool endDown = false;
    // Idle throttle state: the overlay only re-renders when the screen can
    // actually change (see the throttle block below the hotkey poll).
    ULONGLONG lastFrameMs = 0;      // GetTickCount64 of the last drawn frame
    bool prevOpen = g_vape.mainOpen; // detect RSHIFT toggles cheaply

    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Right-Shift toggles panels (vape GUIBind), END quits. Edge-triggered.
        // The window itself stays up; empty areas click through via NCHITTEST.
        // Never while a text field has focus - typing must not toggle the GUI
        // or kill the app. Also never while capturing a module keybind, so
        // the capture key itself can't hide the GUI or quit.
        bool typing = ImGui::GetIO().WantTextInput;
        bool capturing = g_vape.bindingModule != nullptr;
        bool down = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
        if (down && !rshiftDown && !typing && !capturing) {
            g_vape.mainOpen = !g_vape.mainOpen;
            printf("[VapeUI] GUI %s\n", g_vape.mainOpen ? "shown" : "hidden");
        }
        rshiftDown = down;
        bool endDownNow = (GetAsyncKeyState(VK_END) & 0x8000) != 0;
        if (endDownNow && !endDown && !typing && !capturing)
            done = true;
        endDown = endDownNow;
        if (done)
            break;

        // ---- idle throttle --------------------------------------------------
        // This overlay re-presents a fullscreen colorkey layer every refresh;
        // doing that continuously while you play contends with the game for
        // CPU/GPU/DWM time - FPS stays the same, but frame pacing gets visibly
        // rougher. So only render when the screen can change: UI input arrived
        // (WndProc flag), a key edge fired (binds/RSHIFT/END are poll-only while
        // the game has focus), a toggle notification or rainbow cycle is live,
        // or the 100ms safety tick fires to catch anything unmodelled.
        // Otherwise block on the message queue (10ms cap keeps binds snappy)
        // and skip NewFrame/Render/Present entirely.
        {
            bool animating = !g_vape.notifications.empty() ||
                             (g_vape.accent.rainbow && g_vape.accent.rainbowSpeed > 0.0f) ||
                             ImGui::GetIO().WantTextInput || // caret blink while typing
                             g_vape.bindingModule != nullptr;
            bool uiInput = g_uiInput;
            g_uiInput = false;
            bool keyEdge = PollKeyEdges();
            bool openChanged = g_vape.mainOpen != prevOpen;
            prevOpen = g_vape.mainOpen;
            bool safetyTick = GetTickCount64() - lastFrameMs >= 100;
            if (!animating && !uiInput && !keyEdge && !openChanged && !safetyTick) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                continue;
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        g_vape.Render();

        // Clicks landing outside our UI (invisible to ImGui: transparent areas
        // never produce window messages) must drop keyboard capture, or focus
        // stays stuck on us after typing and the underlying app looks frozen.
        {
            static bool lPrev = false, rPrev = false;
            bool lDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            bool rDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            bool clicked = (lDown && !lPrev) || (rDown && !rPrev);
            lPrev = lDown;
            rPrev = rDown;
            if (clicked && !ImGui::GetIO().WantCaptureMouse && g_keyboardCapture)
                ImGui::ClearActiveID();
        }

        // Keyboard stays with the app underneath unless typing in a field.
        SetTypingCapture(ImGui::GetIO().WantTextInput);

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // vsync
        lastFrameMs = GetTickCount64();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(g_hWnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
