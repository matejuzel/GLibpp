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

Both `Debug|x64` and `Release|x64` build. `GLibpp/src/main.cpp` is the app's entry point; swap `Debug` for `Release` in the command above and run `./x64/Release/GLibpp.exe`.

### Tests

The solution holds a second project, **`GLibppTests`** (`GLibppTests/`, x64 only) — a console runner with no framework: `src/TestRunner.h` gives `check()`/`section()` and counts failures, each suite is one `runXxxTests()` called from `src/main.cpp`, exit code 0 means everything passed. The solution build above builds it too; run it with:

```sh
./x64/Debug/GLibppTests.exe
```

It tests the **real** engine headers, not copies of the algorithms — `test_rasterizer.cpp` builds actual `DeviceTargetDIB` targets (a small 40×40 DIB section plus a depth target) and calls the shipping `RasterizerDIB`. The engine's few `.cpp` files it needs (`Mtx4`, `Vec4`, `Mesh`, `MeshFactory`) are compiled straight into the test project; there is no static library. Add new suites by declaring `runXxxTests()` in `TestRunner.h`, calling it from `main.cpp`, and adding the file to `GLibppTests.vcxproj` (+ `.filters`).

There is no lint config and no CMake — MSBuild + the two `.vcxproj` files are the whole build system. The MSVC toolchain here is Czech-localized, so compiler diagnostics come back in Czech.

`main.cpp` asks for monitor `\\.\DISPLAY1`; on a machine without that display name the window silently falls back to the default position, which is fine.

### Adding a file

New `.cpp`/`.h` files must be added to `GLibpp/GLibpp.vcxproj` by hand — MSBuild lists sources explicitly and will not glob. A new directory must also be appended to `AdditionalIncludeDirectories` in **both** the `Debug|x64` and `Release|x64` `ItemDefinitionGroup`s, because includes are **flat**: code writes `#include "Mtx4.h"`, never a relative path. A new engine directory needs the same entry in `GLibppTests/GLibppTests.vcxproj`, where the identical flat list lives once in the `EngineIncludes` user macro.

## Architecture

### Two threads, decoupled by a triple buffer

This is the central design and everything else follows from it.

- **Logic thread** (`App::run`, `src/App/App.h`) — owns the window, pumps `pollEvents()`, and steps `updateLogic()` at a fixed 60 Hz via `TimeManager`. Runs at `THREAD_PRIORITY_HIGHEST`.
- **Render thread** (`Renderer::runLoop`, `src/Engine/Render/Renderer.h`) — renders as fast as it can, unthrottled. `THREAD_PRIORITY_ABOVE_NORMAL`.

They share exactly one thing: `ZeroAllocTripleBuffer<LogicState>`. It is strictly SPSC — the logic thread is the only writer (`publish()`), the render thread the only reader (`update_reader()`). The producer never blocks; the consumer always gets the newest published state and may skip intermediate ones. Do not add a second reader or writer.

Because render rate ≠ logic rate, the renderer **interpolates between two logic states**. It keeps previous/current in a render-thread-local `ZeroAllocStateHistory`, computes alpha from the states' own `tickInfo.lastLogicTick` timestamps, and calls `Slerp(prev, curr, t)`. Both threads share a clock epoch (`TimeManager` constructed with `useGlobalStart=true`) so those timestamps are comparable.

**Consequence for any new scene state:** if you add a field to `Scene` that moves over time, extend the `friend Scene Slerp(...)` in `src/App/Scene.h`. Unhandled fields default to the current state and will visibly stutter. `Scene::Slerp` delegates into the per-type `Lerp`/`Slerp` of `Camera`, `Car`, `CarWheel`, `Vec4`, `Quaternion` — follow that pattern and see the interpolation convention below.

**Shutdown** is one-directional and explicit: `App` and `Renderer` hold *separate* `RunState` instances. ESC stops App's; App's loop then exits and calls `renderer->stop()`, which stops the Renderer's, then joins. The renderer has no way to stop the app.

### Renderer: CRTP, not virtuals

`Renderer<Device>` is a template; the backend is chosen at compile time by the `RENDER_BACKEND_DIB` macro in `App.h` and baked in. `DeviceBase<Device, Target>` (`Backend/Common/DeviceBase.h`) dispatches through `static_cast<Device*>(this)->xxxImpl(...)` — there is no vtable and no runtime backend switching.

A new backend needs: a `GLibpp::Render::DeviceTraits<YourDevice>` specialization (`GpuBuffer3D`/`GpuBuffer2D`/`GpuIndexBuffer`), a target deriving from `DeviceTargetBase<Device>` constructible from a `RenderTargetDescriptor`, a `YourDevice(WindowWin32&)` constructor, and private `...Impl` methods with `friend class DeviceBase<D,T>`. `Renderer` itself only calls `createContext`, `clear`, `drawMesh`, `drawAxis`, `present`, `targetCreate`, `targetResize`, `meshRegister`, `meshUpdate`, `getWindow`. `meshRegisterImpl`/`meshUpdateImpl` have default no-op implementations in `DeviceBase` — a backend that reads canonical data directly implements nothing.

Handles are `StableRegistry<T>::Handle{index, generation}` (both `uint32_t`; a default-constructed handle equals `INVALID`, never slot 0). `targetResize` recreates the object in place **without bumping generation**, so handles survive a resize; only `remove()` invalidates them.

`Renderer::resize` must only ever be called from the render thread. Window resize events arrive on the logic thread, so they go through `resizeRequestSet()` → an atomic `ResizeRequest` the render loop consumes at the top of its next iteration. Keep that pattern for anything else crossing into the renderer.

### Resource management: canonical storage + per-backend residency

- **`GLibpp::Assets::ResourceManager`** (`src/Engine/Assets/ResourceManager.h`) is a **non-templated** pure store owned by **`App`** (assets are application content): `StableRegistry<Mesh>` + `StableRegistry<MeshInstance>` behind the handle aliases from `ResourceHandles.h` (`MeshHandle`, `MeshInstanceHandle`, `MESH_HANDLE_INVALID`, `MESH_INSTANCE_HANDLE_INVALID`). It knows nothing about any Device.
- **Registration happens only before the render thread starts** (`App::setupDemoResources`, called in `App::run` before the thread spawns). `Renderer::runLoop` then calls `resources.freeze()` — after that every registration method asserts. Runtime creation from the logic thread is a planned future feature via an SPSC upload queue consumed at the top of `runLoop` (seam is marked there next to `resizeRequest.consume`); `meshRegister` already returns the handle immediately so that queue can be added without changing the API.
- **Upload walk**: at the top of `runLoop` (render thread — a future GL backend has its context live here) the Renderer iterates `resources.meshForEach` and calls `device.meshRegister(h, mesh)`. The DIB backend copies all geometry into big arrays in `RegistryDIB` (`meshPositions`/`meshIndices`) with a `meshRanges` table indexed by `handle.index` — offsets are the backend's private residency detail, never the ResourceManager's.
- **Drawing is handle-based**: `device.drawMesh(ctx, MeshHandle, world, color, wired)`; there is no `const Mesh&` draw path. `Renderer::drawInstance` resolves a `MeshInstance` (mesh handle + baked `localTransform` + color + wireframe; world transform is passed per-draw because logic computes it every tick) and silently skips INVALID handles — the first frames legitimately carry default-constructed Scenes.
- **Textures follow the same canonical/residency split as meshes.** `Assets::TextureData` (ARGB rows-from-top, same layout as the DIB framebuffer) is loaded by `Platform::ImageLoaderWin32` (GDI+ — part of Windows, decodes JPEG/PNG/BMP; no external dependency) and registered into `ResourceManager::textureRegister` before freeze. The upload walk calls `device.textureRegister(h, data)`; the DIB backend creates a `ShaderResource` target (plain pixel vector, **no GDI** — `DeviceTargetDIB::texelStorage`, `framebuffer` points into it) tracked in `registry.textureRefs`. A pass binds a texture with the `SetTexture` draw command (`ctx.texture`); `rasterizeMesh` resolves the handle into `ShaderUniforms::texture/textureWidth/textureHeight` (nullptr = none → shaders fall back to the instance color).
- **`Mesh` has an optional UV attribute** (`uvBuffer`, interleaved `u,v` per vertex; empty = no UVs — backends substitute zeros). The rasterizer interpolates attributes **perspective-correctly**: `u/w`, `v/w`, `1/w` are affine in screen space, so they ride plane equations like depth; `PixelInput` carries `uOverW/vOverW/invW` and the shader divides only when it needs real UVs (see `ShaderTextured`). `TriangleInput` carries per-vertex `u, v, invW` (invW > 0 is guaranteed by the frustum test).
- **Debug fill budget**: filling ~half the window through the `/Od` rasterizer costs more than a 60 Hz frame on the baseline laptop (~30 FPS) — regardless of triangle count or texture. That's why the demo ground stays wireframe and the texture demo object is a small panel. Large filled/textured surfaces are Release-only territory until the rasterizer gets an optimized fill path.
- **`Scene` owns no geometry** — only `SceneRenderables` (instance handles) and transforms; a `static_assert(std::is_trivially_copyable_v<Scene>)` in `Scene.h` enforces it. Publish and Slerp are allocation-free; keep it that way.
- **Dynamic meshes** (the ground wave): mutate canonical data in place on the render thread via `resources.meshGetDynamic(h)` + `MeshFactory::UpdateGridWave`, then re-upload with `device.meshUpdate(h, resources.meshGet(h))`. Do not reintroduce per-frame `MeshFactory::Create*` calls in `renderFrame` — they caused visible frame drops.

### What is stubbed or dead — don't be misled

- **Depth buffer covers filled triangles only.** The DIB depth target is a plain `std::vector<float>` with no GDI at all (`DeviceTargetDIB` branches on `isDepthFormat(descriptor.format)`), cleared to `kDepthFar = 1.0f` and tested "smaller wins" on NDC z — which is exactly linear in screen space after the perspective divide, so `drawTriangle` interpolates it with a plane equation. **Wireframe outlines and `drawAxis` deliberately ignore depth** (debug overlay), so emit order still decides among lines and between lines and fills.
- **Filled triangles rasterize via edge functions on a 1/256-pixel fixed-point grid**, with pixel-center coverage and a top-left fill rule, so a pixel on a shared edge belongs to exactly one of the two triangles. The fixed point is load-bearing, not cosmetic: `E_AB(p) == -E_BA(p)` holds bit-exactly in integers, so the fill rule decides consistently — in float, rounding could hand a boundary pixel to both triangles or to neither. Don't "simplify" this back to float edge functions or to integer vertex coordinates. `GLibppTests/src/test_rasterizer.cpp` pins the invariant (exclusive, gap-free tiling) — it will fail loudly if someone does.
- **Fragment shaders (DIB)** live in `src/Engine/Render/Backend/DIB/Shaders/`. A shader is a stateless struct — `TriCtx` + `setup(const TriangleInput&, const ShaderUniforms&)` (once per triangle) + `shade(const TriCtx&, const PixelInput&, const ShaderUniforms&)` (once per pixel) — enforced by the C++20 `FragmentShader` concept in `FragmentShader.h`. Dispatch is deliberately two-level: `rasterizeTriangleT<Shader>` inlines `shade` into the inner loop (no per-pixel indirection, same idea as llvmpipe's loop specialization), and `RasterizerDIB::fragmentFunction(id)` returns the instantiation from a constexpr LUT — one indirect call per draw. Adding a shader = new header in `Shaders/` + a `FragmentShaderId` row + a LUT row; a `static_assert` on `Count` keeps enum and table in sync (same pattern as the DrawCommand dispatch). The pass's shader is selected by the `SetShader` draw command (`ctx.fragmentShader`, default `Lambert` — the demo's look; `Solid` is the exact-color passthrough the raster tests rely on; `UvDebug` visualizes normalized screen coordinates; `Textured` samples the bound texture nearest-neighbor on perspective-correct UVs). `ShaderUniforms` are per-draw constants filled by `rasterizeMesh` (currently `invWidth`/`invHeight`) — `PixelInput` deliberately carries **pixel** x/y (exact integer identity, like `gl_FragCoord`); shaders derive normalized u/v from the uniforms, each choosing its own convention. Wireframe outlines take their color from `shade()` at the centroid but still ignore depth. Lighting constants live in `ShaderLambert`, not in `rasterizeMesh` — the geometry stage only supplies the flat view-space normal.
- **No triangle clipping.** `rasterizeMesh` (`DeviceDIB.h`) does an all-or-nothing per-vertex frustum test and skips any triangle with an outside vertex. Only `drawAxisImpl` clips properly.
- **`Renderer::resize` failure clobbers handles.** If `targetResize` fails it returns `TARGET_INVALID`, which overwrites the stored handle permanently — known footgun, documented in a comment, fix out of scope so far.
- **`GLibpp/_old/`** is a previous iteration of the whole engine, kept for reference. Never edit it or copy patterns from it.

## Conventions

Comments and commit messages are in **Czech**; match that when editing existing code. Commit subjects follow `vyvoj - <co>`.

### Namespaces: directory = namespace, rooted at `GLibpp`

Engine code lives in a namespace matching its directory: `GLibpp::Core` (`Engine/Core`, incl. `Datastruct/`, `Input/`), `GLibpp::Geometry`, `GLibpp::Assets`, `GLibpp::Physics`, `GLibpp::Render` (`Engine/Render`), `GLibpp::Platform` (`Platform/Win32`), `GLibpp::Math` (free functions). Cross-subsystem references inside the engine use relative qualification (`Geometry::Mesh`, `Assets::MeshHandle`, `Core::TimeManager`) — lookup ascends to the common `GLibpp` root.

Two deliberate exceptions to the rule: **math types** (`Mtx4`, `Vec4`, `Quaternion`) are global for ergonomics — they appear in nearly every file; and the **application layer** (`App`, `Scene`, `Car`, `LogicState` in `src/App/`) is global, being the leaf consumer. `App.h` pulls short names in via file-level `using` declarations — fine there, don't do that in engine headers.

Project skills in `.claude/skills/` use the **`glib-` prefix** (`glib-add-file`, `glib-check-encoding`, `glib-commit`) so they never collide with built-in skill names (`run`, `review`, `init`, ...). Names that are an upstream protocol contract are the deliberate exception and stay canonical: `verifier-gui` follows the bundled `/verify` protocol's `verifier-*` convention (checked first as the repo's evidence-capture recipe); the same would apply to future `run-*` skills or a persisted `verify` recipe.

### Source encoding — read this before editing any file with Czech comments

Everything under `GLibpp/src/` is **UTF-8 with BOM**, and `/utf-8` is set on both x64 configurations. Keep the BOM when writing these files.

This matters because the history here was mixed: most files were Windows-1250 (the Czech ANSI codepage) with no BOM. A UTF-8 editing tool reading such a file decodes every diacritic as `U+FFFD` and writing it back **destroys the text permanently** — `Změří čas` becomes `Zm��� �as`, and the original bytes are gone. If you ever see `�` in a file, don't edit it: recover the line from git (`git show HEAD:<path>` decoded as CP1250) first.

Two places are still CP1250 on purpose or by neglect: `GLibpp/_old/` (reference-only, never compiled, never edit it) and the `worktree-remove_jitter` branch, whose `Renderer.h` already has mojibake committed.

Math (`src/Math/`) is row-vector style with chained mutating builders — `Mtx4::Identity().translate(x,y,z).rotateY(a)` — and `Quaternion` for orientation. `Mtx4::Identity()` returns a value, so the chain mutates a temporary, not a shared identity.

### Interpolation: hidden friends found by ADL, never static members

Every interpolatable type exposes its interpolation as a **`friend` function defined inline in the class** — `friend T Lerp(const T& a, const T& b, float t)` and/or `Slerp` — so **every call site is unqualified**: `Lerp(a, b, t)`, `Slerp(a, b, t)`, never `T::Lerp(a, b, t)`. Overload resolution picks the right one by argument type (ADL). Types following this today: `Vec4` (Lerp + Slerp), `Quaternion` (Slerp), `Mtx4` (Slerp), `CarWheel` (Lerp), `Camera` (Lerp), `Car` (Slerp), `Scene` (Slerp). The canonical explanation lives at the top of the interpolation block in `src/Math/Vec4.h`.

Why this and not static members: the operation is symmetric in `a`/`b` so it doesn't belong inside one type's scope; unqualified calls make the delegation chain `Scene` → `Car` → `Quaternion`/`Vec4` read uniformly and let generic code interpolate a `T` it doesn't know the name of; and a friend defined in-class is invisible to ordinary lookup, so global-namespace math types add no name collisions.

**Name honestly** — `Slerp` only where something is genuinely spherically interpolated (a quaternion, a basis), `Lerp` otherwise. Don't add a `Slerp` that just forwards to `Lerp`; `Camera` had one and it was deleted.

`App.h` contains `if (0)` / `if (false)` scratch blocks used as ad-hoc debug probes. They're intentional; leave them unless asked.
