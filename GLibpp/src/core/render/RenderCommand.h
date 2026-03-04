#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <thread>

#include "utils/datastruct/TripleBuffer.h"
#include "geometry/Mesh.h"


class MeshRegistry {
public:

	void registerMesh(Mesh* mesh, uint32_t meshId) {
		if (meshId >= meshes.size())
			meshes.resize(meshId + 1);

		meshes[meshId] = mesh;
	}


	Mesh* get(uint32_t id) const {
		return id < meshes.size() ? meshes[id] : nullptr;
	}

private:
	uint32_t nextId = 0;
	std::vector<Mesh*> meshes;
};


extern MeshRegistry g_meshRegistry;

namespace RenderCommand {

	typedef uint8_t uintIndex_t;

	enum class CommandType : uintIndex_t {
		SetClearColor,
		Clear,
		RegisterMesh,
		DrawMesh,
		SetMatrixProjection,
		SetMatrixModelview,
		SetViewport,
	};

	struct SetClearColor {
		int r, g, b;
	};
	struct Clear {
		// empty
	};
	struct RegisterMesh {
		uint32_t meshId;
		Mesh* mesh;
	};
	struct DrawMesh {
		uint32_t meshId;
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
			SetClearColor setClearColor;
			Clear clear;
			RegisterMesh registerMesh;
			DrawMesh drawMesh;
			SetMatrixProjection setMatrixProjection;
			SetMatrixModelView setMatrixModelView;
			SetViewport setViewport;
		};

		Command() : type(CommandType::Clear), clear{} {}
	};


	void execSetClearColor(const Command& cmd);
	void execClear(const Command& cmd);
	void execRegisterMesh(const Command& cmd);
	void execDrawMesh(const Command& cmd);
	void execSetMatrixProjection(const Command& cmd);
	void execSetMatrixModelview(const Command& cmd);
	void execSetViewport(const Command& cmd);

	using Function = void(*)(const Command&);
	extern Function dispatchTable[];

	void exec(const Command& command);


	class Buffer {
	public:

		bool execute() const {

			if (commands.size() == 0) return false;

			for (const auto& cmd : commands) {

				RenderCommand::exec(cmd);
			}
			return true;
		}

		void clear() {
			commands.clear();
		}

		void pushClearColor(int r, int g, int b) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::SetClearColor;
			cmd.setClearColor = { r, g, b };
			push(cmd);
		}

		void pushClear() {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::Clear;
			push(cmd);
		}

		void pushRegisterMesh(Mesh* mesh, uint32_t meshId) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::RegisterMesh;
			cmd.registerMesh = { meshId, mesh };
			push(cmd);
		}


		void pushDrawMesh(uint32_t meshId) {
			RenderCommand::Command cmd;
			cmd.type = RenderCommand::CommandType::DrawMesh;
			cmd.drawMesh = { meshId };
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

		/*inline void push(RenderCommand::Command& command) {
			commands.push_back(command);
		}
		*/

		
	};

}


//extern std::atomic<bool> done;
//extern TripleBuffer<RenderCommand::Buffer> rcb;

//void t_produce_render_comands();


//void t_consume_render_comands();


//void t_produce_and_consume();


