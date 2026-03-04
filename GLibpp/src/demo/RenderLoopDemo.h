#pragma once
#include <Renderer.h>

class RenderLoopDemo
{
public:
	void run() {
	
        // init meshes
        mshCube00.addCube(1.0f).transform(Mtx4::rotationY(0.9f));
        mshCube01.addCube(1.0f).transform(Mtx4::scaling(2.0f, 1.0f, 0.6f));
        mshCube02.addCube(0.5f).transform(Mtx4::translation(1.0f, 0.0f, 1.0f));
        mshCube03.addCube(0.5f).transform(Mtx4::translation(-1.0f, 0.0f, 1.0f));
        mshCube04.addCube(0.5f).transform(Mtx4::translation(-1.0f, 1.0f, 0.0f));
	

        auto& tb = renderer.getRenderCommandBufferRef();

        // write init frame
        {
            auto& writeBuffer = tb.writeBuffer();
            writeBuffer.clear();
            writeBuffer.pushClearColor(0, 0, 0);
            writeBuffer.pushRegisterMesh(&mshCube00, 0);
            writeBuffer.pushRegisterMesh(&mshCube01, 1);
            writeBuffer.pushRegisterMesh(&mshCube02, 2);
            tb.publish();
        }

        // read init frame
        {
            const auto& readBuffer = tb.readBuffer();
            readBuffer.execute();
        }

        // write frame 0
        {
            auto& writeBuffer = tb.writeBuffer();
            writeBuffer.clear();
            writeBuffer.pushClear();
            writeBuffer.pushDrawMesh(0);
            writeBuffer.pushDrawMesh(1);
            writeBuffer.pushDrawMesh(2);
            tb.publish();
        }

        // run reading frames...
        std::thread t_renderLoop(&Renderer::runRenderLoop, &renderer);


        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // write frame 1
        {
            auto& writeBuffer = tb.writeBuffer();
            writeBuffer.clear();
            writeBuffer.pushClear();
            writeBuffer.pushDrawMesh(2);
            tb.publish();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // write frame 2
        {
            auto& writeBuffer = tb.writeBuffer();
            writeBuffer.clear();
            writeBuffer.pushClear();
            writeBuffer.pushDrawMesh(0);
            writeBuffer.pushDrawMesh(2);
            tb.publish();
        }


        {
            RenderCommand::Command cmd;
            cmd.type = RenderCommand::CommandType::RegisterMesh;
            cmd.registerMesh.mesh = &mshCube03;
            cmd.registerMesh.meshId = 3;
            renderer.getRenderCommandQueue().push(cmd);
        }
        {
            RenderCommand::Command cmd;
            cmd.type = RenderCommand::CommandType::RegisterMesh;
            cmd.registerMesh.mesh = &mshCube04;
            cmd.registerMesh.meshId = 4;
            renderer.getRenderCommandQueue().push(cmd);
        }

        // write frame 2
        {
            auto& writeBuffer = tb.writeBuffer();
            writeBuffer.clear();
            writeBuffer.pushClear();
            writeBuffer.pushDrawMesh(3);
            writeBuffer.pushDrawMesh(4);
            tb.publish();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        

        renderer.stop();
        t_renderLoop.join();

	}

    Renderer renderer;
    Mesh mshCube00, mshCube01, mshCube02;

    Mesh mshCube03, mshCube04;
};



