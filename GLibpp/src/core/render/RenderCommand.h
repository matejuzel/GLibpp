#pragma once

#include <cstdint>
#include <vector>
#include <iostream>


namespace RenderCommand {

	typedef uint8_t uintIndex_t;

	enum class CommandType : uintIndex_t {
		SetClearColor,
		Clear,
		//DrawIndexed, //@todo
		//DrawArrays, //@todo
		//SetMatrix, //@todo
	};

	struct CommandSetClearColor {
		int r, g, b;
	};

	struct CommandClear {
	};

	struct Command {

		CommandType type;

		union {
			CommandSetClearColor setClearColor;
			CommandClear clear;
		};
	};

	void execSetClearColor(const Command& cmd) {
		auto& c = cmd.setClearColor;
		std::cout << "Set clear color to: " << c.r << ", " << c.g << ", " << c.b << std::endl;
	}

	void execClear(const Command& cmd) {
		auto& c = cmd.clear;
		std::cout << "Clear screen" << std::endl;
		// do clear
	}

	using CommandFunction = void(*)(const Command&);

	static CommandFunction dispatchTable[] = {
		&execSetClearColor,
		&execClear,
	};

	static void execCommand(const Command & command) {
		// volani funkce z dispatch table podle typu prikazu (LUT - Look-up Table)
		dispatchTable[static_cast<uintIndex_t>(command.type)](command);
	}

}


class RenderCommandBuffer {
public:

	bool execute() const {

		if (commands.size() == 0) return false;

		for (const auto& cmd : commands) {

			RenderCommand::execCommand(cmd);
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


private:
	std::vector<RenderCommand::Command> commands;

	inline void push(RenderCommand::Command& command) {
		commands.push_back(command);
	}
};




