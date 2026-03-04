#pragma once

class GameLoop {
public:

	GameLoop(float logicHz = 60.0f) : logicHz(logicHz) {}	

	bool mainLoopFixedTimestepBufferedAndQueue();

	bool mainLoopFixedTimestamp();
	bool mainLoopBasic();

private:

	float logicHz;
};