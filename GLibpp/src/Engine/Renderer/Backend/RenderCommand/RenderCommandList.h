#pragma once

#include <cstdint>
#include <vector>

#include "RenderCommand.h"
#include "RenderCommandType.h"
#include "handles.h"

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
		const float* raw = matrix.getRawData();
        push<RCMD_SetMatrixProjection>(RenderCommandType::SetMatrixProjection) = { raw };
        return *this;
    }

    RenderCommandList& setMatrixView(const Mtx4& matrix) {
        const float* raw = matrix.getRawData();
        push<RCMD_SetMatrixView>(RenderCommandType::SetMatrixView) = { raw };
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