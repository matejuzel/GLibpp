#pragma once

/*
class Renderer;

#include <cstdint>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <thread>

#include "utils/datastruct/TripleBuffer.h"
#include "geometry/Mesh.h"
#include "window/WindowBuilder.h"
//#include "core/render/Renderer.h"




namespace RenderCommand {

	typedef uint8_t uintIndex_t;

	enum class CommandType : uintIndex_t {
		//SetClearColor,
		Clear,
		RegisterMesh,
		DrawMesh,
		SetMatrixProjection,
		SetMatrixModelview,
		SetViewport,
	};

	
	struct Clear {
		int r, g, b;
	};
	struct RegisterMesh {
		uint32_t meshId;
		Mesh* mesh;
	};
	struct DrawMesh {
		uint32_t meshId;
		Mtx4 transformation;
	};
	struct SetMatrixProjection {
		Mtx4 matrix;
	};
	struct SetMatrixModelView {
		Mtx4 matrix;
	};
	struct SetViewport {
		uint32_t offsetX, offsetY, width, height;
	};


	struct Command {

		CommandType type;

		union {
			//SetClearColor setClearColor;
			Clear clear;
			RegisterMesh registerMesh;
			DrawMesh drawMesh;
			SetMatrixProjection setMatrixProjection;
			SetMatrixModelView setMatrixModelView;
			SetViewport setViewport;
		};

		Command() : type(CommandType::Clear), clear{} {}
	};


	//void execSetClearColor(const Command& cmd, Renderer& renderer);
	void execClear(const Command& cmd, Renderer& renderer);
	void execRegisterMesh(const Command& cmd, Renderer& renderer);
	void execDrawMesh(const Command& cmd, Renderer& renderer);
	void execSetMatrixProjection(const Command& cmd, Renderer& renderer);
	void execSetMatrixModelview(const Command& cmd, Renderer& renderer);
	void execSetViewport(const Command& cmd, Renderer& renderer);

	using Function = void(*)(const Command&, Renderer&);
	extern Function dispatchTable[];

	void exec(const Command& command, Renderer& renderer);


	class Buffer {
	public:

		bool execute(Renderer& renderer) const {

			if (commands.size() == 0) return false;

			for (const auto& cmd : commands) {

				RenderCommand::exec(cmd, renderer);
			}
			return true;
		}

		void clear() {
			commands.clear();
		}

		void pushRegisterMesh(Mesh* mesh, uint32_t meshId) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::RegisterMesh;
			cmd.registerMesh = { meshId, mesh };
			push(cmd);
		}

		void pushClear(int r, int g, int b) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::Clear;
			cmd.clear = { r, g, b };
			push(cmd);
		}

		void pushDrawMesh(uint32_t meshId, Mtx4 transformation) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::DrawMesh;
			cmd.drawMesh = { meshId, transformation };
			push(cmd);
		}

		void pushSetMatrixProjection(Mtx4 matrix) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::SetMatrixProjection;
			cmd.setMatrixProjection = { matrix };
			push(cmd);
		}

		void pushSetMatrixModelView(Mtx4 matrix) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::SetMatrixModelview;
			cmd.setMatrixModelView = { matrix };
			push(cmd);
		}

		void pushSetViewport(uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::SetViewport;
			cmd.setViewport = { offsetX, offsetY, width, height };
			push(cmd);
		}

		inline void push(const Command& command) {
			commands.push_back(command);
		}

	private:
		std::vector<RenderCommand::Command> commands;


		
	};

}

*/

