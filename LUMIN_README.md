# Lumin — liquid glass menu (UI only)

Built on liquidDX11 (Dear ImGui + Direct3D 11 + DXGI Desktop Duplication, Windows x64).

Screens: license key -> product menu (sidebar + Product Information + action button) -> success screen.

## Build
Visual Studio 2022 (C++ workload, v143, Windows SDK 10.0.22621+), x64:

    MSBuild liquidDX11.sln /p:Configuration=Release /p:Platform=x64
    examples\example_win32_directx11\Release\example_win32_directx11.exe

Keep the `data/` folder (fonts, icons) next to the repo layout - the exe searches upward for it.

## Demo behaviour
- Valid key: `LUMIN-DEMO` (any other key shakes the window and shows "Invalid license key").
- The action button is a stub: fake progress bar, then the success screen. It does nothing else.
- Esc closes, F11 toggles capture visibility.

## Where to edit
- `examples/example_win32_directx11/src/app/lumin_ui.inc` - all of the UI (texts, products, colors, sizes).
- Accent colour: `accent_*` in `glass.cfg` (written next to the exe on first run).
