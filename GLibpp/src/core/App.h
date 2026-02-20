#pragma once

class App
{
public:

	static App& instance() {
		static App inst;
		return inst;
	}

	App() = default;

	void work();
	void onKeydown(int keyCode);

private:


};
