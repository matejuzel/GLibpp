#pragma once
#include <memory>

#include "WindowWin32.h"

#include "Mathematics.h"
#include "Mtx4.h"

struct WheelTransformation {

    WheelTransformation(float zPos, float xPos)
        : position(Vec4(xPos, 0.0f, zPos, 1.0f))
    {
    }

    void doSteer(float dAngle)
    {
        steerAngle += dAngle;

        if (steerAngle > steerAngleMax) steerAngle = steerAngleMax;
        if (steerAngle < -steerAngleMax) steerAngle = -steerAngleMax;
    }

    void doResetSteer(float factor, float dt)
    {
        // kolik se má vrátit za tento frame
        float delta = factor * steerAngleMax * dt;

        if (fabs(steerAngle) <= delta) {
            steerAngle = 0.0f;
        }
        else {
            steerAngle -= copysign(delta, steerAngle);
        }
    }


    void doRoll(float dAngle) {
        rollAngle += dAngle;
    }

    Mtx4 get() const {

        auto m_pos = Mtx4::Identity().translate(position.x, position.y, position.z);
        auto m_steer = Mtx4::Identity().rotateY(steerAngle);
        auto m_roll = Mtx4::Identity().rotateX(rollAngle);

        return m_pos * m_steer * m_roll;
    }

private:

    Vec4 position;

    float steerAngleMax = GLibpp::Math::deg2rad(35.0f);
    float steerAngle = 0.0f; // rad 

    float rollAngle = 0.0f;

};

struct CarTransformation {
    
    float speed = 0.0f; //  m / s
    float wheelRadius = 0.5f;

    void speedUp(float dSpeed) 
    {
        speed += dSpeed;

        if (abs(speed) < 0.01) speed = 0.0f;
    }


    void steerFrontWheels(float dAngle) {
        wheelFrontLeft.doSteer(dAngle);
        wheelFrontRight.doSteer(dAngle);
    }

    void steerFrontWheelsReset(float dt) {
        wheelFrontLeft.doResetSteer(4.0f, dt);
        wheelFrontRight.doResetSteer(4.0f, dt);
    }

    void run(float dt) {

        object.translate(0.0f, 0.0f, speed * dt);
        rollAllWheels(speed * dt / wheelRadius);
    }

    void rollAllWheels(float dAngle) {
        wheelFrontLeft.doRoll(dAngle);
        wheelFrontRight.doRoll(dAngle);
        wheelBackLeft.doRoll(dAngle);
        wheelBackRight.doRoll(dAngle);
    }

    CarTransformation()
        : object(Mtx4::Identity()) 
        , wheelFrontLeft(1.0f, -0.5f)
        , wheelFrontRight(1.0f, 0.5f)
        , wheelBackLeft(-1.0f, -0.5f)
        , wheelBackRight(-1.0f, 0.5f)
    {
    }

    Mtx4 object;

    WheelTransformation wheelFrontLeft;
    WheelTransformation wheelFrontRight;
    WheelTransformation wheelBackLeft;
    WheelTransformation wheelBackRight;

    Mtx4 getFrontLeft() const {
        return object * wheelFrontLeft.get();
    }
    Mtx4 getFrontRight() const {
        return object * wheelFrontRight.get();
    }
    Mtx4 getBackLeft() const {
        return object * wheelBackLeft.get();
    }
    Mtx4 getBackRight() const {
        return object * wheelBackRight.get();
    }
};


#include "Scene.h"
#include "ZeroAllocTripleBuffer.h"
using LogicStateBuffered = ZeroAllocTripleBuffer<LogicState>;
#include "Renderer.h"
#include "RenderCommandList.h"
#include "Camera.h"
#include "RunState.h"
#include "RenderTargetDescriptor.h"
#include <atomic>
#include <thread>
#include "TimeManager.h"
#include "Input.h"
#include <random>

#define RENDER_BACKEND_DIB


#if defined(RENDER_BACKEND_DIB)
#include "DeviceDIB.h"
using RenderDevice = Render::DeviceDIB;
#elif defined(RENDER_BACKEND_STENCIL)
#include "DeviceStencil.h"
using RenderDevice = RenderDeviceStencil;
#else
#error "Neni definovan backend!"
#endif

class App {
private:

    using RendererEngine = Render::Renderer<RenderDevice>;

    std::unique_ptr<WindowWin32> window;

	//std::unique_ptr<RendererEngine> renderer; // obecny renderer, ktery pouziva RenderDevice (DIB, Stencil, ...), ktery se zvoli definici makra RENDER_BACKEND_XXX
    std::unique_ptr<Render::Renderer<Render::DeviceDIB>> renderer; // pro vyvoj pouzijeme takhle explicitne, kvuli napovidani v IDE...

    //Scene scene;

    bool fullscreen = false;
    RunState running;

    GLibpp::Input input;

    float logicHz = 120;

    bool checkWindowInitialized() const {
        if (window.get() == nullptr) {
            throw std::runtime_error("Window is not initialized");
        }
        return true;
	}


    LogicStateBuffered logicStateBuffered;

public:

    App() = default;
	~App() = default;

	void setFullscreenMode(bool fullscreen) { 
		checkWindowInitialized();
		window->setFullscreenMode(fullscreen);
    }

    void initialize(uint32_t width, uint32_t height, const std::wstring& preferedDisplayName = L"")
    {
        {
            // WINDOW
            window = std::make_unique<WindowWin32>(width, height, false);
            
            if (!window->build(preferedDisplayName)) {
                throw std::runtime_error("Failed to create window");
            }

            if (fullscreen)
            {
                window->removeOverlapProperty();
                window->resizeWindowToFillScreen();
                window->hideCursor();
            }

            window->setOnCloseCallback([this]() {
                running.stop();
            });

            window->setKeyCallback([this](KeyMap key, bool pressed) {
                onKeyCallback(key, pressed);
            });

            window->setOnResizeCallback([this](uint32_t width, uint32_t height) {
                if (renderer)
                {
                    renderer->resizeRequestSet(width, height);
                    std::cout << "resized: " << width << " ; " << height << std::endl;
                }
            });

            window->glibRegisterRawInputDevices();
        }

        {
            // RENDERER
            renderer = std::make_unique<RendererEngine>(*window, logicStateBuffered, logicHz);
        }

        {
            //scene.camera = Camera::Demo(45);
            //scene.modelMatrix = Mtx4::Identity();

            //renderer->updateScene(scene);
            //renderer->updateScene(scene);
        }

	}

    void lagTest(int ms)
    {
        // Simulace náročné logiky, která trvá ms milisekund
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		std::cout << "Lag test: Simulated work for " << ms << " ms" << std::endl;
	}

    void onKeyCallback(KeyMap key, bool pressed) 
    {

        input.keyboard.setKeyState(static_cast<unsigned char>(key), pressed);

        if (key == KeyMap::KEY_ESC && pressed) {
            running.stop();
        }
	}

    void processInputs()
    {
    }

    void updateLogic(float dt, Scene& scene)
    {

		//std::cout << "Updating logic with dt = " << dt << " seconds" << std::endl;

        //scene.camera.position.x += 10.0f * static_cast<float>(dt);
        //scene.camera.position.y += 10*sinf(0.1f * static_cast<float>(dt));


        scene.modelMatrix2.rotateZ(GLibpp::Math::deg2rad(360.0f * dt));
        scene.modelMatrix2.rotateY(GLibpp::Math::deg2rad(60.0f * dt));
        

        if (input.keyboard.isDown(KeyMap::KEY_ENTER)) {
			scene.modelMatrix.rotateY(1.0f * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_SPACE)) {
            scene.modelMatrix.rotateZ(GLibpp::Math::deg2rad(360.0f * dt));
        }

        if (input.keyboard.isDown(KeyMap::KEY_UP)) {
            scene.car.speedUp(3 * dt);
        }
        else {
            scene.car.speedUp(-0.5 * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_DOWN)) {
            scene.car.speedUp(-2 * dt);
        }
        else {
            scene.car.speedUp(0.5 * dt);
        }


        scene.car.run(dt);


        if (scene.speed > -0.01f && scene.speed < 0.01f) scene.speed = 0.0f;
        if (scene.speed > 0) scene.speed -= 0.001f;
        if (scene.speed < 0) scene.speed += 0.001f;

        scene.matrixVehicle.translate(0, 0, scene.speed * dt);
        /*scene.matrixWheel01.translate(0, 0, scene.speed);
        scene.matrixWheel02.translate(0, 0, scene.speed);
        scene.matrixWheel03.translate(0, 0, scene.speed);
        scene.matrixWheel04.translate(0, 0, scene.speed);*/


        float wheelRadius = 0.5f;
        float angle = -(scene.speed * dt) / wheelRadius;


        scene.matrixWheel01.rotateY(angle);
        scene.matrixWheel02.rotateY(angle);
        scene.matrixWheel03.rotateY(angle);
        scene.matrixWheel04.rotateY(angle);

        bool flagResetSteer = true;
        if (input.keyboard.isDown(KeyMap::KEY_LEFT)) {
            scene.matrixVehicle.rotateY(1.0f * dt);
            scene.matrixSteer.rotateX(dt * 0.5);

            scene.car.steerFrontWheels(dt * 0.5);
            flagResetSteer = false;
        }

        if (input.keyboard.isDown(KeyMap::KEY_RIGHT)) {
            scene.matrixVehicle.rotateY(-1.0f * dt);
            scene.matrixSteer.rotateX(- dt * 0.5);

            scene.car.steerFrontWheels(-dt * 0.5);

            flagResetSteer = false;
        }

        if (flagResetSteer) scene.car.steerFrontWheelsReset(dt);
        

		scene.modelMatrix.rotateY(scene.rotationSpeed * dt);
		scene.camera.position.z += scene.cameraSpeed * dt;

        if (input.keyboard.isDown(KeyMap::KEY_ENTER)) {
            scene.test += 1.0f;
        }
        
        //std::cout << "test inc(1): " << scene.test << std::endl;

    }

    void scenePublish(const LogicState& logicState, double lastLogicTick) {
        
        auto& writeBuffer = logicStateBuffered.get_write_buffer();
        writeBuffer = logicState;
        writeBuffer.tickInfo.lastLogicTick = lastLogicTick;
        logicStateBuffered.publish();
	}

    void run()
    {

        {
            //auto& rm = renderer->getResourceManager();
            //auto texHandle = rm.targetCreate(Render::RenderTargetDescriptor::FramebufferRGBA32bit(window->getClientWidth(), window->getClientHeight()));
            //auto meshInstHandle = rm.meshRegister(MeshInstance());
        }

		// logic scheduler - bude volat updateLogic() s pevnou frekvenci, nezavisle na renderovani
        TimeManager timer(logicHz, true);
        TimeManager timer10Hz(10.0f);

        //Scene scene;

        LogicState logicState;

        logicState.scene.camera = Camera::Demo(45);
        logicState.scene.modelMatrix = Mtx4::Identity();
        logicState.scene.modelMatrix2 = Mtx4::Identity().translate(2.1f, 0.0f, 0.0f);

        std::cout << logicState.scene.modelMatrix.toString() << std::endl;
        std::cout << logicState.scene.modelMatrix2.toString() << std::endl;

        running.start();

        logicStateBuffered.publish();

        std::thread renderThread([this]() {
            renderer->runLoop();
            });

        while (running.isRunning())
        {
            window->pollEvents();

            timer.tickAndDispatchAction([&](double dt, double lastLogicTick) {
                input.keyboard.update();
                updateLogic(static_cast<float>(dt), logicState.scene);
                scenePublish(logicState, lastLogicTick);
             });

            timer10Hz.tickAndDispatchAction([&](double dt) {
                
                /*
                // test jitteru App logiky
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 10);
                int random = dist(rng);
                lagTest(random);
                */
			});


            /*
            if (false) {
                timerOneSecond.tickAndDispatchAction([&](double dt) {
                    double fps = timer.getFps();
                    double low1 = timer.getLow1Percent();
                    double low01 = timer.getLowPoint1Percent();
                    window->setTitle(std::format(
                        "FPS: {:.2f}, 1% Low: {:.2f}, 0.1% Low: {:.2f}",
                        fps, low1, low01
                    ));
                    });
            }
            */

            //timer.waitUntilNextStep(); // to ted delat nebudeme
        }

        renderer->stop();

        if (renderThread.joinable())
        {
            renderThread.join();
        }
    }
};



    /*
    void render()
    {
        // create render target using descriptor helper
        
        renderer->runLoop(
            RenderCommandList::Create()
            .clear({ 232,232,232,255 })
            .setViewport(0, 0, window->getClientWidth(), window->getClientHeight())
            .setMatrixProjection(Mtx4::Perspective(
                45.0f,
                window->getClientWidth() / (float)window->getClientHeight(),
                0.01f,
                1000.0f
            ))
            .setMatrixView(Mtx4::LookAt(
                Vec4(1.0f, 1.0f, 1.0f, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 0.0f),
                Vec4(0.0f, 1.0f, 0.0f, 1.0f)
            ))
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .setMatrixView(Mtx4::Identity())
            .bindMesh(MeshHandle{ 2 }, MaterialHandle{ 2 })
            .draw()
        );
    }*/


