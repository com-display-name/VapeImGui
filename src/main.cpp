#include <Windows.h>
#include <cctype>
#include <d3d11.h>
#include <dwmapi.h>

#include <cstdio>
#include <string>
#include <vector>

#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/imgui_impl_dx11.h"
#include "../third_party/imgui/imgui_impl_win32.h"
#include "../third_party/imgui/imgui_internal.h"
#include "Texture.h"
#include "VapeGui.h"

static ID3D11Device* Device = nullptr;
static ID3D11DeviceContext* DeviceContext = nullptr;
static IDXGISwapChain* SwapChain = nullptr;
static ID3D11RenderTargetView* RenderTarget = nullptr;
static HWND OverlayWindow = nullptr;
static bool OpaqueMode = false;
static bool CapturingKeyboard = false;
static HWND PreviousFocusOwner = nullptr;

static bool PendingUiInput = false;

static bool CreateGraphicsDevice(HWND Window);
static void ReleaseGraphicsDevice();
static void CreateRenderTarget();
static void ReleaseRenderTarget();
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam,
                                                             LPARAM LParam);

static LRESULT WINAPI WndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    if (ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam))
        return true;

    // Empty-area mouse motion does not require another frame.
    if (Message != WM_NCHITTEST)
        PendingUiInput = true;
    switch (Message) {
    case WM_NCHITTEST: {
        bool OverPanel = false;
        if (OpaqueMode) {
            OverPanel = true;
        } else {
            int ScreenX = (int)(short)LOWORD(LParam);
            int ScreenY = (int)(short)HIWORD(LParam);
            POINT Point = {ScreenX, ScreenY};
            ScreenToClient(Window, &Point);
            for (const HitRect& Bounds : Gui.HitRects) {
                if ((float)Point.x >= Bounds.X0 && (float)Point.x < Bounds.X1 &&
                    (float)Point.y >= Bounds.Y0 && (float)Point.y < Bounds.Y1) {
                    OverPanel = true;
                    break;
                }
            }
        }

        static bool WasOverPanel = false;
        if (OverPanel != WasOverPanel) {
            WasOverPanel = OverPanel;
            PendingUiInput = true;
        }
        return OverPanel ? HTCLIENT : HTTRANSPARENT;
    }
    case WM_PAINT:

        ValidateRect(Window, nullptr);
        return 0;
    case WM_SIZE:
        if (Device != nullptr && WParam != SIZE_MINIMIZED) {
            ReleaseRenderTarget();
            SwapChain->ResizeBuffers(0, (UINT)LOWORD(LParam), (UINT)HIWORD(LParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((WParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(Window, Message, WParam, LParam);
}

struct OverlayMonitor {
    std::wstring Device;
    std::string Label;
    RECT Rect = {0, 0, 0, 0};
    RECT Work = {0, 0, 0, 0};
    bool Primary = false;
};
static std::vector<OverlayMonitor> Monitors;
static bool InsetMode = false;

static BOOL CALLBACK MonitorEnumProc(HMONITOR MonitorHandle, HDC, LPRECT, LPARAM) {
    MONITORINFOEXW MonitorInfo;
    MonitorInfo.cbSize = sizeof(MonitorInfo);
    if (!GetMonitorInfoW(MonitorHandle, &MonitorInfo))
        return TRUE;
    OverlayMonitor Monitor;
    Monitor.Device = MonitorInfo.szDevice;
    Monitor.Rect = MonitorInfo.rcMonitor;
    Monitor.Work = MonitorInfo.rcWork;
    Monitor.Primary = (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    char Narrow[64] = {0};
    WideCharToMultiByte(CP_UTF8, 0, MonitorInfo.szDevice, -1, Narrow, (int)sizeof(Narrow), nullptr, nullptr);
    const char* ShortName = Narrow;
    if (strncmp(ShortName, "\\\\.\\", 4) == 0)
        ShortName += 4;
    int Width = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
    int Height = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;
    char Label[128];
    snprintf(Label, sizeof(Label), "#%d %s - %dx%d%s", (int)Monitors.size() + 1, ShortName, Width, Height,
             Monitor.Primary ? " (Primary)" : "");
    Monitor.Label = Label;
    Monitors.push_back(Monitor);
    return TRUE;
}

static std::string GetExecutableDirectory() {
    char Path[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, Path, MAX_PATH);
    std::string Text = Path;
    size_t Separator = Text.find_last_of("\\/");
    return (Separator == std::string::npos) ? "." : Text.substr(0, Separator);
}

static void SaveMonitorChoice(const std::wstring& Device) {
    char Narrow[64] = {0};
    WideCharToMultiByte(CP_UTF8, 0, Device.c_str(), -1, Narrow, (int)sizeof(Narrow), nullptr, nullptr);
    std::string ConfigPath = GetExecutableDirectory() + "\\display.cfg";
    FILE* File = nullptr;
    if (fopen_s(&File, ConfigPath.c_str(), "w") == 0 && File) {
        fprintf(File, "%s\n", Narrow);
        fclose(File);
    }
}

static std::string LoadMonitorChoice() {
    std::string ConfigPath = GetExecutableDirectory() + "\\display.cfg";
    FILE* File = nullptr;
    if (fopen_s(&File, ConfigPath.c_str(), "r") != 0 || !File)
        return "";
    char Line[128] = {0};
    std::string Result;
    if (fgets(Line, (int)sizeof(Line), File)) {
        Result = Line;
        while (!Result.empty() && (Result.back() == '\n' || Result.back() == '\r' || Result.back() == ' '))
            Result.pop_back();
    }
    fclose(File);
    return Result;
}

static std::string ToLower(std::string Text) {
    for (auto& Character : Text)
        Character = (char)tolower((unsigned char)Character);
    return Text;
}

static void UpdateWindowBounds() {
    if (!OverlayWindow || Monitors.empty())
        return;
    int Index = Gui.MonitorIndex;
    if (Index < 0 || Index >= (int)Monitors.size())
        Index = 0;
    const OverlayMonitor& Monitor = Monitors[Index];
    RECT Bounds = Monitor.Rect;
    if (InsetMode) {
        Bounds.top = Monitor.Rect.top + 32;
        Bounds.bottom = Monitor.Work.bottom;
        if (Bounds.bottom < Bounds.top + 100)
            Bounds.bottom = Bounds.top + 100;
    }
    SetWindowPos(OverlayWindow, Gui.AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, Bounds.left, Bounds.top,
                 Bounds.right - Bounds.left, Bounds.bottom - Bounds.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

static void MoveToMonitor(int Index) {
    if (Index < 0 || Index >= (int)Monitors.size() || !OverlayWindow)
        return;
    Gui.MonitorIndex = Index;
    UpdateWindowBounds();
    SaveMonitorChoice(Monitors[Index].Device);
}

static void LoadIcons() {
    const char* Files[] = {
        "combat.png",
        "blatant.png",
        "render.png",
        "utility.png",
        "world.png",
        "inventory.png",
        "friends.png",
        "profiles.png",
        "settings.png",
        "search.png",
        "downexpand.png",
        "expandarrow.png",
        "settingdots.png",
        "discord.png",
        "editlarge.png",
        "vape.png",
        "vapelogo.png",
        "vapelogomini.png",
        "v4mini.png",
        "v4.png",
        "close.png",
        "noti_info.png",
        "noti_alert.png",
        "noti_warning.png",
        "textgui.png",
        "add.png",
        "pin.png",
        "backmini.png",
        "closetiny.png",
        "bind.png",
        "legit_switch.png",
        "legit_mode_icon.png",
        "min.png",
        "back.png",
        "overlaydots.png",
        "range.png",
        "rangeindicator.png",
    };
    int Loaded = 0;
    for (auto Filename : Files) {
        std::string Path = FindIconPath(Filename);
        if (Path.empty())
            continue;
        ID3D11ShaderResourceView* ShaderResource = nullptr;
        int Width = 0, Height = 0;
        if (LoadTextureFromFile(Path, Device, &ShaderResource, &Width, &Height)) {
            Gui.SetIcon(Filename, ShaderResource, Width, Height);
            Loaded++;
        }
    }
    printf("[VapeUI] loaded %d/%d icons\n", Loaded, (int)(sizeof(Files) / sizeof(Files[0])));
    if (Loaded == 0)
        printf("[VapeUI] WARNING: no icons found - check assets/icons next to the vcxproj\n");
}

static bool PollKeyEdges() {
    static const int Keys[] = {
        VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_SHIFT, VK_CONTROL, VK_MENU, VK_ESCAPE, VK_SPACE, VK_RSHIFT,
        VK_END,     '0',        '1',        '2',      '3',        '4',     '5',       '6',      '7',
        '8',        '9',        'A',        'B',      'C',        'D',     'E',       'F',      'G',
        'H',        'I',        'J',        'K',      'L',        'M',     'N',       'O',      'P',
        'Q',        'R',        'S',        'T',      'U',        'V',     'W',       'X',      'Y',
        'Z',        VK_F1,      VK_F2,      VK_F3,    VK_F4,      VK_F5,   VK_F6,     VK_F7,    VK_F8,
        VK_F9,      VK_F10,     VK_F11,     VK_F12,
    };
    static bool Previous[sizeof(Keys) / sizeof(Keys[0])] = {};
    bool Edge = false;
    for (int I = 0; I < (int)(sizeof(Keys) / sizeof(Keys[0])); ++I) {
        bool Down = (GetAsyncKeyState(Keys[I]) & 0x8000) != 0;
        if (Down != Previous[I]) {
            Previous[I] = Down;
            Edge = true;
        }
    }
    return Edge;
}

// Return keyboard focus as soon as text entry ends.
static void SetTypingCapture(bool WantText) {
    if (WantText == CapturingKeyboard)
        return;
    CapturingKeyboard = WantText;
    LONG_PTR ExtendedStyle = GetWindowLongPtr(OverlayWindow, GWL_EXSTYLE);
    if (WantText)
        ExtendedStyle &= ~WS_EX_NOACTIVATE;
    else
        ExtendedStyle |= WS_EX_NOACTIVATE;
    SetWindowLongPtr(OverlayWindow, GWL_EXSTYLE, ExtendedStyle);
    SetWindowPos(OverlayWindow, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (WantText) {
        HWND Previous = GetForegroundWindow();
        if (Previous && Previous != OverlayWindow)
            PreviousFocusOwner = Previous;
        DWORD ForegroundThread = Previous ? GetWindowThreadProcessId(Previous, nullptr) : 0;
        DWORD CurrentThread = GetCurrentThreadId();
        BOOL Attached = (ForegroundThread && ForegroundThread != CurrentThread)
                            ? AttachThreadInput(CurrentThread, ForegroundThread, TRUE)
                            : FALSE;
        BringWindowToTop(OverlayWindow);
        SetForegroundWindow(OverlayWindow);
        SetFocus(OverlayWindow);
        if (Attached)
            AttachThreadInput(CurrentThread, ForegroundThread, FALSE);
    } else if (PreviousFocusOwner) {
        HWND Owner = PreviousFocusOwner;
        PreviousFocusOwner = nullptr;
        if (IsWindow(Owner) && Owner != OverlayWindow)
            SetForegroundWindow(Owner);
    }
}

static bool CreateGraphicsDevice(HWND Window) {
    DXGI_SWAP_CHAIN_DESC SwapDescription = {};
    SwapDescription.BufferCount = 2;
    SwapDescription.BufferDesc.Width = 0;
    SwapDescription.BufferDesc.Height = 0;
    SwapDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapDescription.BufferDesc.RefreshRate.Numerator = 60;
    SwapDescription.BufferDesc.RefreshRate.Denominator = 1;
    SwapDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    SwapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapDescription.OutputWindow = Window;
    SwapDescription.SampleDesc.Count = 1;
    SwapDescription.SampleDesc.Quality = 0;
    SwapDescription.Windowed = TRUE;
    SwapDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT CreateDeviceFlags = 0;
    D3D_FEATURE_LEVEL FeatureLevel;
    const D3D_FEATURE_LEVEL FeatureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT Result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreateDeviceFlags, FeatureLevelArray, 2,
        D3D11_SDK_VERSION, &SwapDescription, &SwapChain, &Device, &FeatureLevel, &DeviceContext);
    if (Result == DXGI_ERROR_UNSUPPORTED) {
        Result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, CreateDeviceFlags,
                                               FeatureLevelArray, 2, D3D11_SDK_VERSION, &SwapDescription,
                                               &SwapChain, &Device, &FeatureLevel, &DeviceContext);
    }
    if (FAILED(Result))
        return false;
    CreateRenderTarget();
    return true;
}

static void ReleaseGraphicsDevice() {
    ReleaseRenderTarget();
    if (SwapChain) {
        SwapChain->Release();
        SwapChain = nullptr;
    }
    if (DeviceContext) {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
    if (Device) {
        Device->Release();
        Device = nullptr;
    }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* BackBuffer = nullptr;
    SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
    if (BackBuffer) {
        Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTarget);
        BackBuffer->Release();
    }
}

static void ReleaseRenderTarget() {
    if (RenderTarget) {
        RenderTarget->Release();
        RenderTarget = nullptr;
    }
}

static int SelectStartupMonitor(const char* CommandLine) {
    int MonitorIndex = -1;
    for (int I = 0; I < (int)Monitors.size(); ++I)
        if (Monitors[I].Primary) {
            MonitorIndex = I;
            break;
        }
    if (MonitorIndex < 0)
        MonitorIndex = 0;
    std::string Saved = LoadMonitorChoice();
    if (!Saved.empty()) {
        for (int I = 0; I < (int)Monitors.size(); ++I) {
            char Narrow[64] = {0};
            WideCharToMultiByte(CP_UTF8, 0, Monitors[I].Device.c_str(), -1, Narrow, (int)sizeof(Narrow),
                                nullptr, nullptr);
            if (Saved == Narrow) {
                MonitorIndex = I;
                break;
            }
        }
    }

    return MonitorIndex;
}

static void InitializeInterface() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io = ImGui::GetIO();
    Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    Io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 8.0f;
    Style.FrameRounding = 6.0f;
    Style.CircleTessellationMaxError = 0.1f;
    Style.ScrollbarRounding = 3.0f;
    Style.WindowBorderSize = 0.0f;
    Style.FrameBorderSize = 0.0f;

    {
        const char* Arial = "C:\\Windows\\Fonts\\arial.ttf";
        if (GetFileAttributesA(Arial) != INVALID_FILE_ATTRIBUTES)
            Io.Fonts->AddFontFromFileTTF(Arial, 15.0f);
        else
            Io.Fonts->AddFontDefault();
    }

    ImGui_ImplWin32_Init(OverlayWindow);
    ImGui_ImplDX11_Init(Device, DeviceContext);
}

static void ConnectWindowSettings() {
    Gui.OnMonitorChange = [](int Index) { MoveToMonitor(Index); };
    Gui.OnTopmostChange = [](bool Top) {
        if (OverlayWindow)
            SetWindowPos(OverlayWindow, Top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    };
    Gui.Fullscreen = true;
    Gui.OnFullscreenChange = [](bool On) {
        InsetMode = !On;
        UpdateWindowBounds();
    };
    Gui.OnStreamproofChange = [](bool On) {
        if (!OverlayWindow)
            return;

        if (!SetWindowDisplayAffinity(OverlayWindow, On ? 0x11 : 0x0))
            printf("[VapeUI] SetWindowDisplayAffinity failed (%lu)\n", GetLastError());
    };
}

static void ReleaseTypingOnOutsideClick() {
    static bool LeftWasDown = false, RightWasDown = false;
    bool LeftIsDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool RightIsDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool Clicked = (LeftIsDown && !LeftWasDown) || (RightIsDown && !RightWasDown);
    LeftWasDown = LeftIsDown;
    RightWasDown = RightIsDown;
    if (Clicked && !ImGui::GetIO().WantCaptureMouse && CapturingKeyboard)
        ImGui::ClearActiveID();
}

static void RunMessageLoop() {
    const ImVec4 ClearColor =
        OpaqueMode ? ImVec4(0.039f, 0.039f, 0.043f, 1.0f) : ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    bool Done = false;
    bool RightShiftWasDown = false;
    bool EndWasDown = false;

    ULONGLONG LastFrameMs = 0;
    bool WasOpen = Gui.MainOpen;

    while (!Done) {
        MSG Message;
        while (::PeekMessage(&Message, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&Message);
            ::DispatchMessage(&Message);
            if (Message.message == WM_QUIT)
                Done = true;
        }
        if (Done)
            break;

        bool Typing = ImGui::GetIO().WantTextInput;
        bool Capturing = Gui.BindingModule != nullptr;
        bool Down = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
        if (Down && !RightShiftWasDown && !Typing && !Capturing) {
            Gui.MainOpen = !Gui.MainOpen;
            printf("[VapeUI] GUI %s\n", Gui.MainOpen ? "shown" : "hidden");
        }
        RightShiftWasDown = Down;
        bool EndIsDown = (GetAsyncKeyState(VK_END) & 0x8000) != 0;
        if (EndIsDown && !EndWasDown && !Typing && !Capturing)
            Done = true;
        EndWasDown = EndIsDown;
        if (Done)
            break;

        {
            // Poll-only binds and animations wake the otherwise idle overlay.
            bool Animating =
                !Gui.Notifications.empty() || (Gui.Accent.Rainbow && Gui.Accent.RainbowSpeed > 0.0f) ||
                ImGui::GetIO().WantTextInput || Gui.BindingModule != nullptr || Gui.IsAnimating();
            bool UiInput = PendingUiInput;
            PendingUiInput = false;
            bool KeyEdge = PollKeyEdges();
            bool OpenChanged = Gui.MainOpen != WasOpen;
            WasOpen = Gui.MainOpen;
            bool SafetyTick = GetTickCount64() - LastFrameMs >= 100;
            if (!Animating && !UiInput && !KeyEdge && !OpenChanged && !SafetyTick) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                continue;
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Gui.Render();

        ReleaseTypingOnOutsideClick();

        SetTypingCapture(ImGui::GetIO().WantTextInput);

        ImGui::Render();
        DeviceContext->OMSetRenderTargets(1, &RenderTarget, nullptr);
        DeviceContext->ClearRenderTargetView(RenderTarget, (float*)&ClearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        SwapChain->Present(1, 0);
        LastFrameMs = GetTickCount64();
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    // Enumerate physical monitor bounds after enabling DPI awareness.
    ImGui_ImplWin32_EnableDpiAwareness();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);
    if (Monitors.empty()) {
        return 1;
    }

    const char* CommandLine = GetCommandLineA();
    std::string LowerCommandLine = ToLower(CommandLine ? CommandLine : "");
    if (LowerCommandLine.find("--opaque") != std::string::npos) {
        OpaqueMode = true;
        printf("[VapeUI] --opaque debug mode: solid background, mouse captured\n");
    }

    // Keep the legacy name to exclude older overlay instances.
    HANDLE InstanceMutex = CreateMutexW(nullptr, FALSE, L"VapeImGuiOverlaySingleton");
    if (!InstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (InstanceMutex)
            CloseHandle(InstanceMutex);
        MessageBoxW(nullptr, L"VapeUI overlay is already running.", L"VapeUI",
                    MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return 0;
    }

    const int MonitorIndex = SelectStartupMonitor(CommandLine);

    for (auto& Monitor : Monitors)
        Gui.MonitorNames.push_back(Monitor.Label);
    Gui.MonitorIndex = MonitorIndex;
    printf("[VapeUI] using %s\n", Monitors[MonitorIndex].Label.c_str());

    WNDCLASSEXW WindowClass = {sizeof(WindowClass),      0,       WndProc, 0L,      0L,
                               GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                               L"VapeUIOverlay",         nullptr};
    ::RegisterClassExW(&WindowClass);
    const RECT& MonitorBounds = Monitors[MonitorIndex].Rect;
    DWORD WindowStyle = WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED;
    OverlayWindow = ::CreateWindowExW(
        WindowStyle, WindowClass.lpszClassName, L"Vape V4", WS_POPUP, MonitorBounds.left, MonitorBounds.top,
        MonitorBounds.right - MonitorBounds.left, MonitorBounds.bottom - MonitorBounds.top, nullptr, nullptr,
        WindowClass.hInstance, nullptr);
    if (!OverlayWindow) {
        printf("[VapeUI] CreateWindowEx failed (%lu)\n", GetLastError());
        ::UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
        CloseHandle(InstanceMutex);
        return 1;
    }

    if (!OpaqueMode) {
        SetLayeredWindowAttributes(OverlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);
        MARGINS Margins = {-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(OverlayWindow, &Margins);
    }

    if (!CreateGraphicsDevice(OverlayWindow)) {
        ReleaseGraphicsDevice();
        ::DestroyWindow(OverlayWindow);
        ::UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
        CloseHandle(InstanceMutex);
        return 1;
    }
    ::ShowWindow(OverlayWindow, SW_SHOWDEFAULT);
    ::UpdateWindow(OverlayWindow);

    InitializeInterface();

    LoadIcons();
    Gui.InitializeModules();
    Gui.Notify("Vape", "Overlay ready - RSHIFT toggles GUI, END quits", 3.0f);

    ConnectWindowSettings();

    RunMessageLoop();

    for (auto& [Name, Icon] : Gui.Icons) {
        if (Icon.ShaderResource)
            Icon.ShaderResource->Release();
    }
    Gui.Icons.clear();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ReleaseGraphicsDevice();
    ::DestroyWindow(OverlayWindow);
    ::UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
    CloseHandle(InstanceMutex);
    return 0;
}

//why would you even read this bad code