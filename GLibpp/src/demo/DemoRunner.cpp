#include "demo/DemoRunner.h"

void DemoRunner::producentConsumentAndDie()
{
	ProducentConsumentDemo demo;
	demo.run();

	std::exit(EXIT_SUCCESS);
}
