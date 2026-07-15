# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

GLibpp is a from-scratch **software 3D renderer** in C++20 on bare Win32 — no GPU API, no external dependencies. Triangles are rasterized on the CPU into a Win32 DIB section and blitted to the window. The current demo is a car driven by a bicycle physics model (arrow keys to drive, ESC to quit).

## Build and run

Only **x64** is usable. The `Win32`/`x86` configurations exist in the solution but have neither `AdditionalIncludeDirectories` nor `LanguageStandard=stdcpp20` set, so they cannot compile. Don't try to fix a build by switching platform.

```sh
# Build (MSBuild path may vary by VS edition)
"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" \
  GLibpp.sln -p:Configuration=Debug -p:Platform=x64 -v:minimal -m

# Run
./x64/Debug/GLibpp.exe
```

There are no tests, no lint config, and no CMake — MSBuild + the `.vcxproj` is the whole build system. The MSVC toolchain here is Czech-localized, so compiler diagnostics come back in Czech.

`main.cpp` asks for monitor `\\.\DISPLAY2`; on a single-monitor machine the window silently falls back to the default position, which is fine.

### Adding a file

New `.cpp`/`.h` files must be added to `GLibpp/GLibpp.vcxproj` by hand — MSBuild lists sources explicitly and will not glob. A new directory must also be appended to `AdditionalIncludeDirectories` in **both** the `Debug|x64` and `Release|x64` `ItemDefinitionGroup`s, because includes are **flat**: code writes `#include "Mtx4.h"`, never a relative path. `HandleRegistry.h` is on disk but absent from the project — untracked by the build.

## Architecture

### Two threads, decoupled by a triple buffer

This is the central design and everything else follows from it.

- **Logic thread** (`App::run`, `src/App/App.h`) — owns the window, pumps `pollEvents()`, and steps `updateLogic()` at a fixed 60 Hz via `TimeManager`. Runs at `THREAD_PRIORITY_HIGHEST`.
- **Render thread** (`Renderer::runLoop`, `src/Engine/Renderer/Renderer.h`) — renders as fast as it can, unthrottled. `THREAD_PRIORITY_ABOVE_NORMAL`.

They share exactly one thing: `ZeroAllocTripleBuffer<LogicState>`. It is strictly SPSC — the logic thread is the only writer (`publish()`), the render thread the only reader (`update_reader()`). The producer never blocks; the consumer always gets the newest published state and may skip intermediate ones. Do not add a second reader or writer.

Because render rate ≠ logic rate, the renderer **interpolates between two logic states**. It keeps previous/current in a render-thread-local `ZeroAllocStateHistory`, computes alpha from the states' own `tickInfo.lastLogicTick` timestamps, and calls `Slerp(prev, curr, t)`. Both threads share a clock epoch (`TimeManager` constructed with `useGlobalStart=true`) so those timestamps are comparable.

**Consequence for any new scene state:** if you add a field to `Scene` that moves over time, extend the `friend Scene Slerp(...)` in `src/App/Scene.h`. Unhandled fields default to the current state and will visibly stutter. `Scene::Slerp` delegates into `CarTransformation`/`WheelTransformation`/`Camera` Slerp/Lerp overloads — follow that pattern.

**Shutdown** is one-directional and explicit: `App` and `Renderer` hold *separate* `RunState` instances. ESC stops App's; App's loop then exits and calls `renderer->stop()`, which stops the Renderer's, then joins. The renderer has no way to stop the app.

### Renderer: CRTP, not virtuals

`Renderer<Device>` is a template; the backend is chosen at compile time by the `RENDER_BACKEND_DIB` macro in `App.h` and baked in. `DeviceBase<Device, Target>` (`Backend/Common/DeviceBase.h`) dispatches through `static_cast<Device*>(this)->xxxImpl(...)` — there is no vtable and no runtime backend switching.

A new backend needs: a `Render::DeviceTraits<YourDevice>` specialization (`GpuBuffer3D`/`GpuBuffer2D`/`GpuIndexBuffer`), a target deriving from `DeviceTargetBase<Device>` constructible from a `RenderTargetDescriptor`, a `YourDevice(WindowWin32&)` constructor, and private `...Impl` methods with `friend class DeviceBase<D,T>`. `Renderer` itself only calls `createContext`, `clear`, `drawMesh`, `drawAxis`, `present`, `targetResize`, `getWindow`.

Handles are `StableRegistry<T>::Handle{index, generation}`. `targetResize` recreates the object in place **without bumping generation**, so handles survive a resize; only `remove()` invalidates them.

`Renderer::resize` must only ever be called from the render thread. Window resize events arrive on the logic thread, so they go through `resizeRequestSet()` → an atomic `ResizeRequest` the render loop consumes at the top of its next iteration. Keep that pattern for anything else crossing into the renderer.

### What is stubbed or dead — don't be misled

- **No depth buffer.** `depthbufferHandle` is created and resized but never bound or read. Triangles draw in index order with no Z-test.
- **No triangle clipping.** `drawMeshImpl` does an all-or-nothing per-vertex frustum test and skips any triangle with an outside vertex. Only `drawAxisImpl` clips properly.
- **`ResourceManager::meshRegister` / `meshInstanceRegister` are `@todo` stubs returning `{0,0}`.** The mesh handles at the top of `Renderer.h` are meaningless. Meshes are currently passed to `drawMesh` by const ref and several are rebuilt from `MeshFactory` *every frame* inside `renderFrame`.
- **`Backend/RenderCommand/` is legacy.** It compiles but nothing references it; the only call site is a commented-out block at the bottom of `App.h`.
- **`GLibpp/_old/`** is a previous iteration of the whole engine, kept for reference. Never edit it or copy patterns from it.

## Conventions

Comments and commit messages are in **Czech**; match that when editing existing code. Commit subjects follow `vyvoj - <co>`.

Math (`src/math/`) is row-vector style with chained mutating builders — `Mtx4::Identity().translate(x,y,z).rotateY(a)` — and `Quaternion` for orientation. `Mtx4::Identity()` returns a value, so the chain mutates a temporary, not a shared identity.

`App.h` contains `if (0)` / `if (false)` scratch blocks used as ad-hoc debug probes. They're intentional; leave them unless asked.
