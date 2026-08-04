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

Both `Debug|x64` and `Release|x64` build. `main.cpp` is the only entry point; swap `Debug` for `Release` in the command above and run `./x64/Release/GLibpp.exe`.

There are no tests, no lint config, and no CMake — MSBuild + the `.vcxproj` is the whole build system. The MSVC toolchain here is Czech-localized, so compiler diagnostics come back in Czech.

`main.cpp` asks for monitor `\\.\DISPLAY2`; on a single-monitor machine the window silently falls back to the default position, which is fine.

### Adding a file

New `.cpp`/`.h` files must be added to `GLibpp/GLibpp.vcxproj` by hand — MSBuild lists sources explicitly and will not glob. A new directory must also be appended to `AdditionalIncludeDirectories` in **both** the `Debug|x64` and `Release|x64` `ItemDefinitionGroup`s, because includes are **flat**: code writes `#include "Mtx4.h"`, never a relative path.

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

A new backend needs: a `Render::DeviceTraits<YourDevice>` specialization (`GpuBuffer3D`/`GpuBuffer2D`/`GpuIndexBuffer`), a target deriving from `DeviceTargetBase<Device>` constructible from a `RenderTargetDescriptor`, a `YourDevice(WindowWin32&)` constructor, and private `...Impl` methods with `friend class DeviceBase<D,T>`. `Renderer` itself only calls `createContext`, `clear`, `drawMesh`, `drawAxis`, `present`, `targetCreate`, `targetResize`, `meshRegister`, `meshUpdate`, `getWindow`. `meshRegisterImpl`/`meshUpdateImpl` have default no-op implementations in `DeviceBase` — a backend that reads canonical data directly implements nothing.

Handles are `StableRegistry<T>::Handle{index, generation}` (both `uint32_t`; a default-constructed handle equals `INVALID`, never slot 0). `targetResize` recreates the object in place **without bumping generation**, so handles survive a resize; only `remove()` invalidates them.

`Renderer::resize` must only ever be called from the render thread. Window resize events arrive on the logic thread, so they go through `resizeRequestSet()` → an atomic `ResizeRequest` the render loop consumes at the top of its next iteration. Keep that pattern for anything else crossing into the renderer.

### Resource management: canonical storage + per-backend residency

- **`Render::ResourceManager`** (`src/Engine/Renderer/ResourceManager.h`) is a **non-templated** pure store owned by **`App`** (assets are application content): `StableRegistry<Mesh>` + `StableRegistry<MeshInstance>` behind the handle aliases from `ResourceHandles.h` (`MeshHandle`, `MeshInstanceHandle`, `MESH_*_INVALID`). It knows nothing about any Device.
- **Registration happens only before the render thread starts** (`App::setupDemoResources`, called in `App::run` before the thread spawns). `Renderer::runLoop` then calls `resources.freeze()` — after that every registration method asserts. Runtime creation from the logic thread is a planned future feature via an SPSC upload queue consumed at the top of `runLoop` (seam is marked there next to `resizeRequest.consume`); `meshRegister` already returns the handle immediately so that queue can be added without changing the API.
- **Upload walk**: at the top of `runLoop` (render thread — a future GL backend has its context live here) the Renderer iterates `resources.meshForEach` and calls `device.meshRegister(h, mesh)`. The DIB backend copies all geometry into big arrays in `RegistryDIB` (`meshPositions`/`meshIndices`) with a `meshRanges` table indexed by `handle.index` — offsets are the backend's private residency detail, never the ResourceManager's.
- **Drawing is handle-based**: `device.drawMesh(ctx, MeshHandle, world, color, wired)`; there is no `const Mesh&` draw path. `Renderer::drawInstance` resolves a `MeshInstance` (mesh handle + baked `localTransform` + color + wireframe; world transform is passed per-draw because logic computes it every tick) and silently skips INVALID handles — the first frames legitimately carry default-constructed Scenes.
- **`Scene` owns no geometry** — only `SceneRenderables` (instance handles) and transforms; a `static_assert(std::is_trivially_copyable_v<Scene>)` in `Scene.h` enforces it. Publish and Slerp are allocation-free; keep it that way.
- **Dynamic meshes** (the ground wave): mutate canonical data in place on the render thread via `resources.meshGetDynamic(h)` + `MeshFactory::UpdateGridWave`, then re-upload with `device.meshUpdate(h, resources.meshGet(h))`. Do not reintroduce per-frame `MeshFactory::Create*` calls in `renderFrame` — they caused visible frame drops.

### What is stubbed or dead — don't be misled

- **No depth buffer.** `depthbufferHandle` (a `Renderer` member) is created and resized but never bound or read. Triangles draw in index order with no Z-test.
- **No triangle clipping.** `rasterizeMesh` (`DeviceDIB.h`) does an all-or-nothing per-vertex frustum test and skips any triangle with an outside vertex. Only `drawAxisImpl` clips properly.
- **`Renderer::resize` failure clobbers handles.** If `targetResize` fails it returns `TARGET_INVALID`, which overwrites the stored handle permanently — known footgun, documented in a comment, fix out of scope so far.
- **`Backend/RenderCommand/` is legacy.** Headers only, referenced by nothing (the `App.h` include was removed); it no longer compiles as part of any TU.
- **`GLibpp/_old/`** is a previous iteration of the whole engine, kept for reference. Never edit it or copy patterns from it.

## Conventions

Comments and commit messages are in **Czech**; match that when editing existing code. Commit subjects follow `vyvoj - <co>`.

### Source encoding — read this before editing any file with Czech comments

Everything under `GLibpp/src/` is **UTF-8 with BOM**, and `/utf-8` is set on both x64 configurations. Keep the BOM when writing these files.

This matters because the history here was mixed: most files were Windows-1250 (the Czech ANSI codepage) with no BOM. A UTF-8 editing tool reading such a file decodes every diacritic as `U+FFFD` and writing it back **destroys the text permanently** — `Změří čas` becomes `Zm��� �as`, and the original bytes are gone. If you ever see `�` in a file, don't edit it: recover the line from git (`git show HEAD:<path>` decoded as CP1250) first.

Two places are still CP1250 on purpose or by neglect: `GLibpp/_old/` (reference-only, never compiled, never edit it) and the `worktree-remove_jitter` branch, whose `Renderer.h` already has mojibake committed.

Math (`src/math/`) is row-vector style with chained mutating builders — `Mtx4::Identity().translate(x,y,z).rotateY(a)` — and `Quaternion` for orientation. `Mtx4::Identity()` returns a value, so the chain mutates a temporary, not a shared identity.

`App.h` contains `if (0)` / `if (false)` scratch blocks used as ad-hoc debug probes. They're intentional; leave them unless asked.
