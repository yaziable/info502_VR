# Shadow Creatures

OpenGL/C++ shadow creature scene for INFO-H-502.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- On Windows: MinGW with `mingw32-make` available in `PATH`
- Internet access for the first CMake configure, because CMake downloads Assimp and ImGui with `FetchContent`

## Build and Run

From the repository root:

```powershell
cmake -S .\scene_shadow_creatures3 -B .\scene_shadow_creatures3\build -G "MinGW Makefiles"
cmake --build .\scene_shadow_creatures3\build
```

Then execute the program with:

```powershell
.\scene_shadow_creatures3\build\ShadowCreatures\ShadowCreatures.exe
```

If you configure directly from the `ShadowCreatures` folder, use:

```powershell
cmake -S .\scene_shadow_creatures3\ShadowCreatures -B .\scene_shadow_creatures3\ShadowCreatures\build -G "MinGW Makefiles"
cmake --build .\scene_shadow_creatures3\ShadowCreatures\build
```

Then execute that build with:

```powershell
.\scene_shadow_creatures3\ShadowCreatures\build\ShadowCreatures.exe
```

Do not commit generated `build` folders. If CMake reports that `CMakeCache.txt` was created in another directory, delete that build folder and configure again.

On PowerShell:

```powershell
Remove-Item -Recurse -Force .\scene_shadow_creatures3\build
cmake -S .\scene_shadow_creatures3 -B .\scene_shadow_creatures3\build -G "MinGW Makefiles"
cmake --build .\scene_shadow_creatures3\build
```
