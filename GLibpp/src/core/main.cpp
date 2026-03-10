#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>

#include "core/Types.h"
#include "core/App.h"
#include "window/WindowBuilder.h"
#include "demo/ProducentConsumentDemo.h"
#include "window/WindowProcedure.h"
#include "demo/DemoRunner.h"
#include "core/render/RenderContext.h"

#include "geometry/Entity.h"

#pragma comment(lib, "User32.lib")

int main()
{

    {
        AssetRegistry<Mesh> meshRegistry;
        AssetRegistry<Material> materialRegistry;

		auto mat01 = materialRegistry.add(std::make_unique<Material>());
		auto mat02 = materialRegistry.add(std::make_unique<Material>());

		auto msh01 = meshRegistry.add(
			Mesh::Net(10)
		);

		auto msh02 = meshRegistry.add(
			Mesh::Cube(1.0f)
		);

		auto msh03 = meshRegistry.add(
			Mesh::Cube(1.0f)
		);

        
		Entity ent01(msh01, 0, Mtx4::identity());


        const auto& a = ent01.getChildren();
        a[0]->transform(Mtx4::rotationX(0.01f));

		ent01.addChild(Entity::Make(msh02, mat01, Mtx4::identity().translate(-1.0f, 0.0f, 0.0f)));
		ent01.addChild(Entity::Make(msh02, mat02, Mtx4::identity().translate(1.0f, 0.0f, 0.0f)));
		ent01.addChild(Entity::Make(msh03, mat02, Mtx4::identity().translate(0.0f, 1.0f, 0.0f).rotateY(2.0f * 3.14159f / 4.0f)));

		auto* mesh = meshRegistry.get(ent01.getMeshHandle());
        
        

		meshRegistry.iterate([](Mesh& mesh) {
			mesh.addCube(1.0f);
		});

    }



    return 0;







    //DemoRunner::renderLoopAndDie();
    //DemoRunner::producentConsumentAndDie();

    int width = 1600;
    int height = 900;

    WindowBuilder wnd(width, height, WindowProc);
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

    Renderer renderer(&wnd);
;
    App::instance().renderer = &renderer;
    App::instance().init();
    App::instance().runGameLoop();

    return 0;
}


