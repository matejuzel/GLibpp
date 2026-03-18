#pragma once

#include <memory>

// dopredne deklarace zavislosti (misto includu)
class WindowWin32;
class Mesh;
class Material;
class Mtx4;

struct MeshHandle {
    uint32_t id;
};

class IRenderDevice {
public:

    virtual ~IRenderDevice() = default; // nutny pro polymorfizmus aby nedoslo k leakum

    virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void setMatrixView(const Mtx4& view) = 0;
    virtual void setMatrixProjection(const Mtx4& projection) = 0;

    virtual MeshHandle registerMesh(const Mesh& mesh) = 0;
    virtual void drawMesh(MeshHandle handle, const Mtx4& modelMatrix, const Material& material) = 0;

    virtual void clear(uint32_t color) = 0;
    virtual void present() = 0;
    
    virtual void draw2dCircle(uint32_t x, uint32_t y, uint32_t size, uint32_t color) = 0; // toto pak dame pryc
};