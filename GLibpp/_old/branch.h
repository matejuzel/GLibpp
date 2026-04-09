#pragma once
#include <vector>
#include <string>
#include <thread>

// BASIC STRUCTURES

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

class Mtx4 {
private:
    float data[16];
public:
    Mtx4(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14, float a15) : {
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

    static Mtx4 Perspective(float fov, float aspect, float near, float far) {
        // @todo
        return Identity();
	}

    static Mtx4 LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
		// @todo
        return Identity();
     }

        
};

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

struct MeshHandle {
    //@todo 
    uint32_t handle;
};
struct MaterialHandle {
    //@todo 
    uint32_t handle;
};

struct DeviceTargetHandle {
    //@todo 
    uint32_t handle;
};

class MaterialRegistry {
    //@todo 
    uint32_t handle;
};

class TextureRegistry {
    //@todo 
    uint32_t handle;
};

class MeshRegistry {
public:
    static Mesh* getMesh(const std::string& name) {
        // @todo
    }
};


class MeshInstance {
public:
	MeshHandle mesh;
	MaterialHandle material;
	Mtx4 transform;
};

struct RCMD_SetViewport {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct RCMD_SetMatrixProjection {
    Mtx4 matrix;
};

struct RCMD_SetMatrixView {
    Mtx4 matrix;
};

struct RCMD_BindMesh {
    MeshHandle mesh;
    MaterialHandle material;
};

struct RCMD_BindTarget {
    DeviceTargetHandle hwnd; // @todo
};

struct RCMD_Draw {
    uint32_t dummy; // nepotrebuji zadna data
};

struct RCMD_Clear {
    Color color; // nepotrebuji zadna data
};

enum class RenderCommandType : uint8_t {
    SetViewport,
    SetMatrixProjection,
    SetMatrixView,
    BindMesh,
    BindTarget,
    Draw,
    Clear,
    // @todo: add more command types as needed
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

struct RenderCommand {
    RenderCommandType type;
    RenderCommandData data;
};


class RenderCommandList {
private:
    std::vector<RenderCommand> commands;

    template<typename T>
    T& push(RenderCommandType type) {

        // Ujisteni ze typ T je trivialnine kopirovatelny a velikost se vejde do RenderCommandData unionu - vyhodnocuje se v compile-time - nema zadny vliv na runtime vykon
        static_assert(std::is_trivially_copyable_v<T>, "Render command must be trivially copyable");
        static_assert(sizeof(T) <= sizeof(RenderCommandData), "Render command too large");
        //

        RenderCommand cmd;
        cmd.type = type;
        commands.push_back(cmd);
        return *reinterpret_cast<T*>(&commands.back().data);
    }

public:
    
    static RenderCommandList Create() {
        return RenderCommandList();
	}

    RenderCommandList& setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        push<RCMD_SetViewport>(RenderCommandType::SetViewport) = {
            x, 
            y,
            width, 
            height,
        };
		return *this;
	}

    RenderCommandList& setMatrixProjection(const Mtx4& matrix) {
        push<RCMD_SetMatrixProjection>(RenderCommandType::SetMatrixProjection) = {
            matrix
        };
        return *this;
    }

    RenderCommandList& setMatrixView(const Mtx4& matrix) {
        push<RCMD_SetMatrixView>(RenderCommandType::SetMatrixView) = {
            matrix
        };
        return *this;
	}

    RenderCommandList& bindMesh(const MeshHandle& mesh, const MaterialHandle& material) {
        push<RCMD_BindMesh>(RenderCommandType::BindMesh) = {
            mesh,
            material
        };
        return *this;
	}

    RenderCommandList& bindTarget(const DeviceTargetHandle& target) {
        push<RCMD_BindTarget>(RenderCommandType::BindTarget) = {
            target
        };
        return *this;    
    }

    RenderCommandList& draw() {
        push<RCMD_Draw>(RenderCommandType::Draw) = {
            0 // dummy
        };
        return *this;
    }

    RenderCommandList& clear(const Color& color) {
        push<RCMD_Clear>(RenderCommandType::Clear) = {
            color
        };
        return *this;
	}

    const std::vector<RenderCommand>& getCommands() const { return commands; }
};


struct mHWND {
    HWND hwnd;
};

struct Viewport {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

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
    Mtx4 projection;
    Mtx4 view;
    MaterialHandle material;
    MeshHandle mesh;
    DeviceTargetHandle target;
};

class IRenderContext {
protected:
    IRenderDevice& device;

	RenderState state = { // toto asi jeste lepe vymyslet, ale pro zacatek to staci
        {0,0,0,0},
        Mtx4::Identity(),
        Mtx4::Identity(),
		MaterialHandle{ 0 },
		MeshHandle{ 0 }
    };
public:

    IRenderContext(IRenderDevice& device) : device(device) {}
    const RenderState& getState() const noexcept { return state; }

    virtual ~IRenderContext() = default;
    virtual void renderCommandList(const RenderCommandList& cmds) = 0;
    virtual void publish() = 0;
};

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;
    virtual std::unique_ptr<IRenderTarget> createRenderTarget(const RenderTargetDescriptor& descriptor) = 0;
    virtual std::unique_ptr<IRenderContext> beginContext(IRenderTarget& target) = 0;
    virtual void drawMesh(IRenderContext& ctx) = 0;
	virtual void clear(IRenderContext& ctx, const Color& color) = 0;
};

// CPU - DIB RENDERER IMPLEMENTATION
class RenderDeviceDIB : public IRenderDevice {
    public:
    std::unique_ptr<IRenderTarget> createRenderTarget(uint32_t w, uint32_t h) override {
        return std::make_unique<RenderTargetDIB>();
    }
    std::unique_ptr<IRenderContext> beginContext(IRenderTarget& target) override {
        return std::make_unique<RenderContextDIB>(*this, target);
    }
    void drawMesh(IRenderContext& ctx) override 
    {
        const auto& state = ctx.getState();
        auto& target = ctx.getTarget();

        // @todo
	}
    void clear(IRenderContext& ctx, const Color& color) override 
    {
        const auto& state = ctx.getState();
        auto& target = ctx.getTarget();
		// @todo
    }
};

class RenderTargetDIB : public IRenderTarget {
	// @todo
};

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device, IRenderTarget& target) : IRenderContext(device, target) {}

    void renderCommandList(const RenderCommandList& cmds) override {

        // replay faze

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
                state.viewport = {
                    cmd.data.setViewport.x,
                    cmd.data.setViewport.y,
                    cmd.data.setViewport.width,
                    cmd.data.setViewport.height
				};
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

};

class ResourceManager {
public:
	MeshRegistry meshRegistry;
    MaterialRegistry materialRegistry;
	TextureRegistry textureRegistry;
    DeviceTargetRegistry deviceTargetRegistry;
};

// RENDERER IMPLEMENTATION
class Renderer {
private:
    IRenderDevice& device;
    //std::unique_ptr<IRenderTarget> target;

    ResourceManager resources;

public:
    Renderer(IRenderDevice& device, uint32_t w, uint32_t h) : device(device) {
        //target = device.createRenderTarget(w, h);
    }

    void runLoop(const RenderCommandList& commandList) {


        device.createRenderTarget(RenderTargetDescriptor::Framebuffer(w,h));

        auto ctx = device.beginContext(*target);
        ctx->renderCommandList(commandList);
        ctx->publish();
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
    {}

    void runRenderingLoop()
    {
        // scene - toto je jen na ukazku, pak to bude kompletne oddeleno do jine struktury

        auto target = device->createRenderTarget(window.getClientWidth(), window.getClientHeight());

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
                .bindMesh(
                    MeshHandle{ 1 },
                    MaterialHandle{ 1 }
                )
                .draw()
                .bindMesh(
                    MeshHandle{ 1 },
                    MaterialHandle{ 1 }
                )
                .draw()
                .setMatrixView(Mtx4::Identity())
                .bindMesh(
                    MeshHandle{ 2 },
                    MaterialHandle{ 2 }
				)
                .draw()
        );
    }
};




/*
EngineApp
 - unique_ptr<WindowWin32> win(x, y, 32)
 - unique_ptr<IRenderDevice> device
 - unique_ptr<Renderer> renderer(*device)

Renderer(IRenderDevice& device)
 - IRenderDevice& device
 - unique_ptr<IRenderTarget> target(HWND hwnd)
 - unique_ptr<IRenderContext> ctx(device, *target)

WindowWin32(uint32_t width, uint32_t height, uint32_t depth)
 - uint32_t width
 - uint32_t height
 - uint32_t depth

IRenderDevice
 - IRenderContext begin()

IRenderContext(IRenderDevice& device, IRenderTarget& target)
 - IRenderDevice& device
 - IRenderTarget& target

IRenderTarget(HWND hwnd)
 - HWND hwnd

*/