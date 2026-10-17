# VapeUI

A Windows overlay rendered with Dear ImGui and DirectX 11.

## Build

Open `VapeUI.slnx` in the parent directory with Visual Studio and build `Release | x64`.
The project uses the v145 C++ toolset and the Windows 10 SDK. Assets live in `assets/icons`.

## Source layout

- `Main.cpp`: window messages, monitor selection, graphics lifetime, and the frame loop.
- `VapeGui.h` / `VapeGui.cpp`: interface state, keybinds, notifications, and render dispatch.
- `VapeData.h` / `VapeModuleData.cpp`: data types and the module catalogue.
- `VapeStyle.h`: colors and layout constants.
- `VapeInternal.h`: shared drawing, panel setup, color, and measurement helpers.
- `VapeModules.cpp`: module rows and option widgets.
- Other `Vape*.cpp` files: individual windows and overlays.
- `Texture.cpp`: image loading and icon lookup.

### Yes this was written by AI
