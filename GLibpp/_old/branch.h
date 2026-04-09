#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <memory>
#include <cmath>
#include <type_traits>
#include <array>
#include <atomic>

// Windows handle (bez externích závislostí pokud nejsme na Win)
#ifdef _WIN32
#include <Windows.h>
#else
using HWND = void*;
#endif

// --- BASIC STRUCTURES ---

template<typename T>
class DoubleBuffer {
public:
    DoubleBuffer() {
        index_.store(0, std::memory_order_relaxed);
    }

    // reference na back buffer (zapis)
    T& writeBuffer() noexcept {
        return buffers_[backIndex()];
    }

    // const reference na front buffer (cteni)
    const T& readBuffer() const noexcept {
        return buffers_[frontIndex()];
    }

    // atomicky prehodit back -> front
    void publish() noexcept {
        // increment index mod 2
        int prev = index_.load(std::memory_order_relaxed);
        int next = (prev + 1) & 1;
        index_.store(next, std::memory_order_release);
    }

    // vrati index front/back (pomocne pro integraci s existujicim API)
    int frontIndex() const noexcept {
        return index_.load(std::memory_order_acquire);
    }

    int backIndex() const noexcept {
        return (frontIndex() + 1) & 1;
    }

private:
    std::array<T, 2> buffers_;
    std::atomic<int> index_; // index front bufferu (0 nebo 1)
};

enum class TextureFormat : uint8_t {
    RGBA32F,
    Depth32F,
    RGBA8,
    RGBA16F,
    Depth24,
    Depth24Stencil8,
};

enum class TextureUsage : uint8_t {
    ColorAttachment,
    DepthAttachment,
    ShaderResource,
    Storage,
};


class RenderTargetRegistry {
    // @todo
    uint32_t toto;
};

struct RenderTargetDescriptor {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
    TextureUsage usage;

    uint8_t samples = 1; // MSAA samples, default 1 (no MSAA)
    uint8_t mipLevels = 1; // number of mip levels, default 1 (no mipmaps)

    static RenderTargetDescriptor Framebuffer(uint32_t width, uint32_t height) {
        return {
            width, height,
            TextureFormat::RGBA32F,
            TextureUsage::ColorAttachment,
            1,1
        };
    }
};

// --- Simple 4x4 matrix helper (self-contained) ---
class Mtx4 {
private:
    float data[16];
public:
    // explicit constructor: row-major order
    Mtx4(float a0, float a1, float a2, float a3,
        float a4, float a5, float a6, float a7,
        float a8, float a9, float a10, float a11,
        float a12, float a13, float a14, float a15)
    {
        data[0] = a0; data[1] = a1; data[2] = a2; data[3] = a3;
        data[4] = a4; data[5] = a5; data[6] = a6; data[7] = a7;
        data[8] = a8; data[9] = a9; data[10] = a10; data[11] = a11;
        data[12] = a12; data[13] = a13; data[14] = a14; data[15] = a15;
    }

    static Mtx4 Identity() {
        return Mtx4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        );
    }

    // fov in degrees
    static Mtx4 Perspective(float fovDeg, float aspect, float near, float far) {
        const float PI = 3.14159265358979323846f;
        const float f = 1.0f / std::tan((fovDeg * PI / 180.0f) * 0.5f);
        float A = f / aspect;
        float B = f;
        float C = far / (far - near);
        float D = -(far * near) / (far - near);

        // row-major:
        return Mtx4(
            A, 0, 0, 0,
            0, B, 0, 0,
            0, 0, C, 1,
            0, 0, D, 0
        );
    }

    static Mtx4 LookAt(float eyeX, float eyeY, float eyeZ,
        float centerX, float centerY, float centerZ,
        float upX, float upY, float upZ)
    {
        // compute forward = normalize(center - eye)
        float fx = centerX - eyeX;
        float fy = centerY - eyeY;
        float fz = centerZ - eyeZ;
        float flen = std::sqrt(fx * fx + fy * fy + fz * fz);
        if (flen == 0.0f) flen = 1.0f;
        fx /= flen; fy /= flen; fz /= flen;

        // up normalize
        float ux = upX, uy = upY, uz = upZ;
        float ulen = std::sqrt(ux * ux + uy * uy + uz * uz);
        if (ulen == 0.0f) ulen = 1.0f;
        ux /= ulen; uy /= ulen; uz /= ulen;

        // right = normalize(cross(f, up))
        float rx = fy * uz - fz * uy;
        float ry = fz * ux - fx * uz;
        float rz = fx * uy - fy * ux;
        float rlen = std::sqrt(rx * rx + ry * ry + rz * rz);
        if (rlen == 0.0f) rlen = 1.0f;
        rx /= rlen; ry /= rlen; rz /= rlen;

        // recompute up = cross(right, forward)
        float ux2 = ry * fz - rz * fy;
        float uy2 = rz * fx - rx * fz;
        float uz2 = rx * fy - ry * fx;

        // row-major lookAt matrix
        return Mtx4(
            rx, ux2, -fx, 0,
            ry, uy2, -fy, 0,
            rz, uz2, -fz, 0,
            -(rx * eyeX + ry * eyeY + rz * eyeZ),
            -(ux2 * eyeX + uy2 * eyeY + uz2 * eyeZ),
            fx * eyeX + fy * eyeY + fz * eyeZ,
            1
        );
    }
};

// --- simple color / material / mesh types ---
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class Material {
    // textures, etc.
};

class Mesh {
    // geometry data
};

struct MeshHandle { uint32_t handle = 0; };
struct MaterialHandle { uint32_t handle = 0; };
struct DeviceTargetHandle { uint32_t handle = 0; };

class MaterialRegistry { uint32_t handle = 0; };
class TextureRegistry { uint32_t handle = 0; };

class MeshRegistry {
public:
    static Mesh* getMesh(const std::string& /*name*/) {
        return nullptr; // @todo implement lookup
    }
};

class MeshInstance {
public:
    MeshHandle mesh;
    MaterialHandle material;
    Mtx4 transform = Mtx4::Identity();
};

// Render command structures...
struct RCMD_SetViewport { uint32_t x; uint32_t y; uint32_t width; uint32_t height; };
struct RCMD_SetMatrixProjection { Mtx4 matrix; };
struct RCMD_SetMatrixView { Mtx4 matrix; };
struct RCMD_BindMesh { MeshHandle mesh; MaterialHandle material; };
struct RCMD_BindTarget { DeviceTargetHandle hwnd; };
struct RCMD_Draw { uint32_t dummy; };
struct RCMD_Clear { Color color; };

enum class RenderCommandType : uint8_t {
    SetViewport,
    SetMatrixProjection,
    SetMatrixView,
    BindMesh,
    BindTarget,
    Draw,
    Clear,
};

union RenderCommandData {
    RCMD_SetViewport setViewport;
    RCMD_SetMatrixProjection setMatrixProjection;
    RCMD_SetMatrixView setMatrixView;
    RCMD_BindMesh bindMesh;
    RCMD_BindTarget bindTarget;
    RCMD_Draw draw;
    RCMD_Clear clear;
    RenderCommandData() {}
    ~RenderCommandData() {}
};

struct RenderCommand { RenderCommandType type; RenderCommandData data; };

class RenderCommandList {
private:
    std::vector<RenderCommand> commands;

    template<typename T>
    T& push(RenderCommandType type) {
        static_assert(std::is_trivially_copyable<T>::value, "Render command must be trivially copyable");
        static_assert(sizeof(T) <= sizeof(RenderCommandData), "Render command too large");
        RenderCommand cmd; cmd.type = type;
        commands.push_back(cmd);
        return *reinterpret_cast<T*>(&commands.back().data);
    }

public:
    static RenderCommandList Create() { return RenderCommandList(); }

    RenderCommandList& setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        push<RCMD_SetViewport>(RenderCommandType::SetViewport) = { x, y, width, height };
        return *this;
    }

    RenderCommandList& setMatrixProjection(const Mtx4& matrix) {
        push<RCMD_SetMatrixProjection>(RenderCommandType::SetMatrixProjection) = { matrix };
        return *this;
    }

    RenderCommandList& setMatrixView(const Mtx4& matrix) {
        push<RCMD_SetMatrixView>(RenderCommandType::SetMatrixView) = { matrix };
        return *this;
    }

    RenderCommandList& bindMesh(const MeshHandle& mesh, const MaterialHandle& material) {
        push<RCMD_BindMesh>(RenderCommandType::BindMesh) = { mesh, material };
        return *this;
    }

    RenderCommandList& bindTarget(const DeviceTargetHandle& target) {
        push<RCMD_BindTarget>(RenderCommandType::BindTarget) = { target };
        return *this;
    }

    RenderCommandList& draw() {
        push<RCMD_Draw>(RenderCommandType::Draw) = { 0 };
        return *this;
    }

    RenderCommandList& clear(const Color& color) {
        push<RCMD_Clear>(RenderCommandType::Clear) = { color };
        return *this;
    }

    const std::vector<RenderCommand>& getCommands() const { return commands; }
};

// lightweight HWND wrapper
struct mHWND { HWND hwnd = nullptr; };

struct Viewport { uint32_t x; uint32_t y; uint32_t width; uint32_t height; };

// WindowWin32 (lightweight)
class WindowWin32 {
private:
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    mHWND hwnd;

public:
    WindowWin32(uint32_t w, uint32_t h, uint32_t d) : width(w), height(h), depth(d) {}
    uint32_t getClientWidth() const { return width; }
    uint32_t getClientHeight() const { return height; }
    mHWND getHwnd() const { return hwnd; }
};

// RENDERING INTERFACES
class IRenderDevice;

class IRenderTarget {
private:
    RenderTargetDescriptor descriptor;
public:
    IRenderTarget(const RenderTargetDescriptor& descriptor) : descriptor(descriptor) {}
    virtual ~IRenderTarget() = default;
};

struct RenderState {
    Viewport viewport = { 0,0,0,0 };
    Mtx4 projection = Mtx4::Identity();
    Mtx4 view = Mtx4::Identity();
    MaterialHandle material;
    MeshHandle mesh;
    DeviceTargetHandle target;
};

class IRenderContext {
protected:
    IRenderDevice& device;
    RenderState state;

public:
    IRenderContext(IRenderDevice& device) : device(device) {}
    const RenderState& getState() const noexcept { return state; }
    virtual ~IRenderContext() = default;
    virtual void renderCommandList(const RenderCommandList& cmds) = 0;
    virtual void publish() = 0;
};

class IRenderDevice {
public:
    virtual DeviceTargetHandle acquireBackbuffer() = 0;
    virtual void present(DeviceTargetHandle) = 0;
    virtual std::unique_ptr<IRenderContext> beginContext(DeviceTargetHandle target) = 0;
    virtual std::unique_ptr<IRenderTarget> createRenderTarget(const RenderTargetDescriptor& descriptor) = 0;
    virtual void drawMesh(IRenderContext& ctx) = 0;
    virtual void clear(IRenderContext& ctx, const Color& color) = 0;
};

// CPU - DIB RENDERER IMPLEMENTATION
class RenderTargetDIB : public IRenderTarget {
public:
    RenderTargetDIB(const RenderTargetDescriptor& desc) : IRenderTarget(desc) {}
    // @todo: actual DIB data
};

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device) : IRenderContext(device) {}

    void renderCommandList(const RenderCommandList& cmds) override {
        for (const auto& cmd : cmds.getCommands()) {
            switch (cmd.type) {
            case RenderCommandType::BindMesh:
                state.mesh = cmd.data.bindMesh.mesh;
                state.material = cmd.data.bindMesh.material;
                break;
            case RenderCommandType::BindTarget:
                state.target = cmd.data.bindTarget.hwnd;
                break;
            case RenderCommandType::SetViewport:
                state.viewport = { cmd.data.setViewport.x, cmd.data.setViewport.y, cmd.data.setViewport.width, cmd.data.setViewport.height };
                break;
            case RenderCommandType::SetMatrixProjection:
                state.projection = cmd.data.setMatrixProjection.matrix;
                break;
            case RenderCommandType::SetMatrixView:
                state.view = cmd.data.setMatrixView.matrix;
                break;
            case RenderCommandType::Draw:
                device.drawMesh(*this);
                break;
            case RenderCommandType::Clear:
                device.clear(*this, cmd.data.clear.color);
                break;
            }
        }
    }

    void publish() override {
        // @todo: present / flush
    }
};

class RenderDeviceDIB : public IRenderDevice {

private:
    struct Framebuffer {
        std::vector<uint32_t> pixels;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // nahradi drivejsi std::array<Framebuffer,2> buffers_;
    DoubleBuffer<Framebuffer> buffers_;

public:
    std::unique_ptr<IRenderTarget> createRenderTarget(const RenderTargetDescriptor& descriptor) override {
        // alokujeme buffery pres writeBuffer()
        auto& fb = buffers_.writeBuffer(); // back buffer pro inicializaci
        fb.width = descriptor.width;
        fb.height = descriptor.height;
        fb.pixels.assign(static_cast<size_t>(fb.width) * fb.height, 0u);

        // take care: i druhy slot musi byt inicializovan
        auto& fb2 = buffers_.readBuffer(); // front slot
        fb2.width = fb.width;
        fb2.height = fb.height;
        fb2.pixels.assign(static_cast<size_t>(fb2.width) * fb2.height, 0u);

        return std::make_unique<RenderTargetDIB>(descriptor);
    }

    // Acquire backbuffer vraci identifikator (zachovano stavajici API)
    DeviceTargetHandle acquireBackbuffer() override {
        return DeviceTargetHandle{ static_cast<uint32_t>(buffers_.backIndex()) };
    }

    // beginContext pouziva target (identifikator), context se bude odkazovat na tento handle
    std::unique_ptr<IRenderContext> beginContext(DeviceTargetHandle target) override {
        return std::make_unique<RenderContextDIB>(*this, target);
    }

    // Present: device bere zodpovednost za prehozeni (publishing) back->front
    void present(DeviceTargetHandle /*target*/) override {
        // jednoduse prehodime (writer jiz do back bufferu napsal a vola present)
        buffers_.publish();
    }

    void clear(IRenderContext& ctx, const Color& color) override {
        const auto& state = ctx.getState();
        // clear na back buffer - pouzijeme handle v state.target
        uint32_t idx = state.target.handle;
        if (static_cast<int>(idx) == buffers_.backIndex()) {
            auto& fb = buffers_.writeBuffer();
            uint32_t packed = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.b) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.r));
            std::fill(fb.pixels.begin(), fb.pixels.end(), packed);
        }
        (void)state;
    }

    void drawMesh(IRenderContext& ctx) override {
        const auto& state = ctx.getState();
        // use state.mesh / state.material / state.viewport / state.target
        // @todo: software draw implementation
        (void)state;
    }
};

// Resource managers
class AssetManager {
    MeshRegistry meshRegistry;
    MaterialRegistry materialRegistry;
    TextureRegistry textureRegistry;
};

class RenderResourceManager {
public:
    RenderResourceManager() = default;
    RenderTargetRegistry renderTargetRegistry;
};

// RENDERER IMPLEMENTATION
class Renderer {
private:
    IRenderDevice& device;
    RenderResourceManager resources;
    uint32_t width = 0;
    uint32_t height = 0;

public:
    Renderer(IRenderDevice& device, uint32_t width, uint32_t height)
        : device(device), width(width), height(height) {
    }

    void runLoop(const RenderCommandList& commandList) {
		auto backbuffer = device.acquireBackbuffer();
        auto ctx = device.beginContext(backbuffer);
        ctx->renderCommandList(commandList);
        ctx->publish();

        device.present(backbuffer);
    }
};

class EngineApp {
private:
    std::unique_ptr<IRenderDevice> device;
    WindowWin32 window;
    Renderer renderer;

public:
    EngineApp()
        : device(std::make_unique<RenderDeviceDIB>())
        , window(800, 600, 32)
        , renderer(*device, window.getClientWidth(), window.getClientHeight())
    {
    }

    void runRenderingLoop()
    {
        // create render target using descriptor helper
        auto target = device->createRenderTarget(RenderTargetDescriptor::Framebuffer(window.getClientWidth(), window.getClientHeight()));

        renderer.runLoop(
            RenderCommandList::Create()
            .clear({ 0,0,0,255 })
            .setViewport(0, 0, window.getClientWidth(), window.getClientHeight())
            .setMatrixProjection(Mtx4::Perspective(
                45.0f,
                window.getClientWidth() / (float)window.getClientHeight(),
                0.01f,
                1000.0f
            ))
            .setMatrixView(Mtx4::LookAt(
                1.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f
            ))
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .setMatrixView(Mtx4::Identity())
            .bindMesh(MeshHandle{ 2 }, MaterialHandle{ 2 })
            .draw()
        );
    }
};