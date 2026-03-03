#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>

#include "core/App.h"
#include "window/WindowBuilder.h"
#include "utils/datastruct/SPSCQueue.h"
#include "demo/ProducentConsumentDemo.h"
#include "window/WindowProcedure.h"

#include "demo/DemoRunner.h"
#include <TripleBuffer.h>
#include <RenderCommand.h>

#pragma comment(lib, "User32.lib")


TripleBuffer<RenderCommandBuffer> rcb;
std::atomic <bool> done{ false };

void t_produce_render_comands() 
{

    for (int i = 0; i < 10; i++) 
    {
        auto& buffProducent = rcb.writeBuffer();
        buffProducent.clear();
        buffProducent.pushClearColor(255, 0, 0);
        buffProducent.pushClear();
        rcb.publish();
    }

}

void t_consume_render_comands() 
{
    while (1) {
		const auto& buffConsument = rcb.readBuffer();
        buffConsument.execute();

        if (done.load()) break;
    }
}


int main()
{

	//t_produce_render_comands();

    std::thread t1(t_produce_render_comands);
	std::thread t2(t_consume_render_comands);
    
	Sleep(1000); // Simulace nějaké práce, během které se budou produkovat příkazy  
    done.store(true);

    t1.join();
    t2.join();
    
	std::cout << "Render commands produced and consumed." << std::endl;


    return 0;

    //DemoRunner::producentConsumentAndDie();

    auto& app = App::instance();

    WindowBuilder wnd(1366, 800, WindowProc);

    if (!wnd.build())
    {
        std::cerr << "Window build failed" << std::endl;
        return 1;
    }

    if (!wnd.DIB_init()) 
    {
		std::cerr << "DIB_init failed" << std::endl;
        return 1;
    }

    app.setWindow(&wnd);
    app.runGameLoop();

    return 0;
}


