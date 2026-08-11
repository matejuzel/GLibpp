#pragma once

#include "Mtx4.h"
#include "Color.h"
#include "Viewport.h"
#include "FragmentShaderId.h"
#include "ResourceHandles.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace GLibpp::Render {

    // ---- draw commandy ----
    //
    // Zaklad command streamu render vlakna (extract -> submit vzor):
    // build faze prelozi interpolovanou Scene na linearni seznam commandu,
    // submit faze ho prehraje do Device pres dispatch tabulku (viz Renderer).
    //
    // Tagged union s fixni velikosti (~80 B, dominuje Mtx4) - zadne pointery,
    // linearni prehravani je cache-friendly. TargetHandle je per-device typ,
    // proto je command sablona na Device (konzistentni s CRTP render stackem).
    //
    // Vedome rozhodnuti: Set* commandy delaji stream stavovym - budouci razeni
    // drawu (sort key, painter's algorithm misto chybejiciho Z-bufferu) smi
    // prehazovat jen souvisle runy DrawMesh mezi state commandy.
    //
    // Otevrena dvirka:
    //  - sort key (uint64) vedle commandu + razeni runu drawu
    //  - budouci hierarchicky graf (flat uzly {lokalni TRS, parent index})
    //    emituje do tehoz DrawListu - submit faze se nemeni
    //  - dalsi druhy commandu (UpdateMesh pro dynamicke meshe, ...)
    template <typename Device>
    struct DrawCommand {

        // poradi enumu = poradi dispatch tabulky v Rendereru (hlida static_assert)
        enum class Kind : uint8_t {
            DrawMesh,
            DrawAxis,
            SetView,
            SetProjection,
            SetViewport,
            SetFramebuffer,
            SetDepthbuffer,
            Clear,
            SetShader,
            SetTexture,
            Count // pocet druhu - neni to command
        };

        struct DrawMeshPayload { Mtx4 world; Assets::MeshInstanceHandle instance; };
        struct DrawAxisPayload { Mtx4 world; };
        struct MatrixPayload   { Mtx4 matrix; };                          // SetView i SetProjection
        struct ViewportPayload { Viewport viewport; };
        struct TargetPayload   { typename Device::TargetHandle target; }; // SetFramebuffer i SetDepthbuffer
        struct ClearPayload    { Color color; };
        struct ShaderPayload   { FragmentShaderId shader; };
        struct TexturePayload  { Assets::TextureHandle texture; };

        Kind kind = Kind::DrawMesh;
        union {
            DrawMeshPayload drawMesh{}; // NSDMI -> union ma default ctor (pole v DrawListu)
            DrawAxisPayload drawAxis;
            MatrixPayload   matrix;
            ViewportPayload viewport;
            TargetPayload   target;
            ClearPayload    clear;
            ShaderPayload   shader;
            TexturePayload  texture;
        };
    };

    // Linearni zero-alloc seznam commandu s pevnou kapacitou ("retained scene,
    // immediate draw list" - kazdy frame se resetuje a plni znovu).
    // Plni se VYHRADNE pres typovane emit metody, aby se tag a payload nerozjely.
    // Plny list: assert v debugu, v release se command tise zahodi.
    template <typename Device, size_t N>
    class DrawList {

    public:

        using Command = DrawCommand<Device>;
        using Kind = typename Command::Kind;
        using TargetHandle = typename Device::TargetHandle;

        static_assert(std::is_trivially_copyable_v<Command>,
            "command musi zustat trivialne kopirovatelny - zadne pointery/vlastnictvi");

        // ---- emit API (build faze) ----

        void drawMesh(const Mtx4& world, Assets::MeshInstanceHandle instance) {
            if (Command* c = push(Kind::DrawMesh)) c->drawMesh = { world, instance };
        }

        void drawAxis(const Mtx4& world) {
            if (Command* c = push(Kind::DrawAxis)) c->drawAxis = { world };
        }

        void setView(const Mtx4& matrix) {
            if (Command* c = push(Kind::SetView)) c->matrix = { matrix };
        }

        void setProjection(const Mtx4& matrix) {
            if (Command* c = push(Kind::SetProjection)) c->matrix = { matrix };
        }

        void setViewport(const Viewport& viewport) {
            if (Command* c = push(Kind::SetViewport)) c->viewport = { viewport };
        }

        void setFramebuffer(TargetHandle target) {
            if (Command* c = push(Kind::SetFramebuffer)) c->target = { target };
        }

        void setDepthbuffer(TargetHandle target) {
            if (Command* c = push(Kind::SetDepthbuffer)) c->target = { target };
        }

        void clear(const Color& color) {
            if (Command* c = push(Kind::Clear)) c->clear = { color };
        }

        void setShader(FragmentShaderId shader) {
            if (Command* c = push(Kind::SetShader)) c->shader = { shader };
        }

        void setTexture(Assets::TextureHandle texture) {
            if (Command* c = push(Kind::SetTexture)) c->texture = { texture };
        }

        // ---- sprava listu / iterace (submit faze) ----

        void reset() noexcept { count = 0; }

        size_t size() const noexcept { return count; }

        static constexpr size_t capacity() noexcept { return N; }

        const Command* begin() const noexcept { return commands; }
        const Command* end() const noexcept { return commands + count; }

    private:

        Command* push(Kind kind) noexcept {
            assert(count < N && "DrawList je plny - zvys kapacitu (kDrawListCapacity)");
            if (count >= N) return nullptr;
            Command& c = commands[count++];
            c.kind = kind;
            return &c;
        }

        Command commands[N];
        size_t count = 0;
    };

}
