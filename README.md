# Less Clutter - source

Less Clutter is an SKSE plugin that removes (or locks in place) useless items and empty containers from the Skyrim world. Author: mlw05. Licence: GPL-3.0-or-later with exceptions; see COPYING, EXCEPTIONS.md and LICENSING.md. Third-party notices: THIRD-PARTY-NOTICES.md.

## Layout

- `src/Plugin.cpp` - the whole plugin
- `CMakeLists.txt` - build definition
- `LessClutter.ini` - default configuration shipped in the mod
- `tools/Build.ps1` - build script
- `licenses/` - third-party licence texts

## Build

Requires Windows x64, Visual Studio 2026 Community with Desktop development with C++, its CMake component, the Windows SDK, and:

- CommonLibSSE-NG (alandtse/CommonLibVR) at commit `70c1acd5261210982bd52f6d4468a082fe04d798`
- vcpkg with `spdlog:x64-windows-static` installed

`tools/Build.ps1` takes `-Reference <folder>` pointing at a folder that contains `CommonLibSSE-NG` and `vcpkg` (default: `D:\Development\high-contrast\build`). Change the default to your own paths. It outputs `build/Release/LessClutter.dll` and stages `dist/LessClutter/SKSE/Plugins/`.

Run it with `powershell -ExecutionPolicy Bypass -File tools\Build.ps1`.

The plugin targets Steam Skyrim 1.7.104.0 with SKSE64 2.3.1 and Address Library only.

## Behaviour

See the comments in `LessClutter.ini`. In short: items whose display name is in `[Names]` (or whose base form is in `[Forms]`) are removed or locked per `Mode`; containers outside player homes that are empty or hold only those items are handled per `EmptyContainers`. Player-dropped, script-spawned and quest-aliased references are never touched.
