#include "demo/DemoRunner.h"
#include "RenderLoopDemo.h"

void DemoRunner::producentConsumentAndDie()
{
	ProducentConsumentDemo demo;
	demo.run();
	std::exit(EXIT_SUCCESS);
}

void DemoRunner::renderLoopAndDie()
{
	RenderLoopDemo demo;
	demo.run();
	std::exit(EXIT_SUCCESS);
}
