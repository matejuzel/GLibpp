#include "RenderCommand.h"

MeshRegistry g_meshRegistry;

namespace RenderCommand {

    void execSetClearColor(const Command& cmd) {
        auto& c = cmd.setClearColor;
        std::cout << "Set clear color to: " << c.r << ", " << c.g << ", " << c.b << std::endl;
    }

    void execClear(const Command& cmd) {
        std::cout << "Clear screen\n";
    }

    void execRegisterMesh(const Command& cmd) {
        g_meshRegistry.registerMesh(cmd.registerMesh.mesh, cmd.registerMesh.meshId);
        std::cout << "Registered mesh with ID: " << cmd.registerMesh.meshId << std::endl;
    }

    void execDrawMesh(const Command& cmd) {
        Mesh* mesh = g_meshRegistry.get(cmd.drawMesh.meshId);
        if (!mesh) {
            std::cout << "Mesh ID not found: " << cmd.drawMesh.meshId << std::endl;
            return;
        }
		std::cout << "Drawing mesh with ID: " << cmd.drawMesh.meshId << std::endl;
        // draw mesh...
    }

    Function dispatchTable[] = {
        &execSetClearColor,
        &execClear,
        &execRegisterMesh,
        &execDrawMesh,
    };

    void exec(const Command& command) {
        dispatchTable[(uintIndex_t)command.type](command);
    }

}

std::atomic<bool> done{ false };
TripleBuffer<RenderCommand::Buffer> rcb;

Mesh meshCube01, meshCube02, meshCube03;

void t_produce_render_comands() {

	meshCube01.addCube(1.0f).transform(Mtx4::rotationY(0.9f));
    meshCube02.addCube(1.0f).transform(Mtx4::scaling(2.0f, 1.0f, 0.6f));
    meshCube03.addCube(0.5f).transform(Mtx4::translation(1.0f, 0.0f, 1.0f));

    auto& buff = rcb.writeBuffer();

    buff.pushRegisterMesh(&meshCube01, 0);
    buff.pushRegisterMesh(&meshCube02, 1);
    buff.pushRegisterMesh(&meshCube03, 2);
	rcb.publish();

    if (false)
    for (int i = 0; i < 10; ++i) {
        auto& buff = rcb.writeBuffer();
        buff.clear();

        //buff.pushClearColor(255, 0, 0);
        //buff.pushClear();

        
		buff.pushDrawMesh(0);
        buff.pushDrawMesh(1);
        buff.pushDrawMesh(2);
        
        rcb.publish();
    }
}

void t_consume_render_comands() {
    while (!done.load()) {
        const auto& buff = rcb.readBuffer();
        buff.execute();   // tady se volá execRegisterMesh -> registrace do registry
    }
}

void t_produce_and_consume() {
    std::thread producer(t_produce_render_comands);
    std::thread consumer(t_consume_render_comands);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    done.store(true);
    producer.join();
    consumer.join();
}

