#pragma once
#include <memory>

#include "WindowWin32.h"

#include "Mathematics.h"
#include "Mtx4.h"
#include "Vec4.h"
#include "Mesh.h"
#include "Quaternion.h"
#include "BicycleModel.h"
#include "MeshFactory.h"

struct WheelTransformation {

    WheelTransformation(float zPos, float xPos, float radius)
        : position(Vec4(xPos, 0.0f, zPos, 1.0f))
        , mesh(MeshFactory::CreateCylinder(radius, 0.4, 12).applyTransformation(Mtx4::RotationZ(3.14159f / 2.0f)))
    {
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

    Mesh getMesh() const { return mesh; }

    static WheelTransformation Lerp(const WheelTransformation& a, const WheelTransformation& b, float t) {
    
        auto interpolated = b;

        interpolated.steerAngle = a.steerAngle + (b.steerAngle - a.steerAngle) * t;
        interpolated.rollAngle = a.rollAngle + (b.rollAngle - a.rollAngle) * t;

        return interpolated;
    }

    float steerAngle = 0.0f; // rad 
    float rollAngle = 0.0f;
private:

    Mesh mesh;
    Vec4 position;

    float steerAngleMax = GLibpp::Math::deg2rad(35.0f);
    
};


struct CarTransformation
{
    // fyzikální model – poloha = střed zadní nápravy
    GLibpp::Physics::BicycleModel model;

    // wheel transformace
    WheelTransformation wheelFrontLeft;
    WheelTransformation wheelFrontRight;
    WheelTransformation wheelBackLeft;
    WheelTransformation wheelBackRight;

    // transformace auta (původ = střed zadní nápravy)
    Mesh mesh;
    
    CarTransformation()
        : model(GLibpp::Physics::BicycleModel::Basic())
        , wheelFrontLeft(model.params.wheelBase, -model.params.wheelTrack * 0.5f, model.params.wheelRadius)
        , wheelFrontRight(model.params.wheelBase, model.params.wheelTrack * 0.5f, model.params.wheelRadius)
        , wheelBackLeft(0, -model.params.wheelTrack * 0.5f, model.params.wheelRadius)
        , wheelBackRight(0, model.params.wheelTrack * 0.5f, model.params.wheelRadius)
        //, object(Mtx4::Identity())
        , mesh(MeshFactory::CreateCylinder(1.0f, 6, 16).applyTransformation(Mtx4::RotationX(3.14159f / 2.0f)*Mtx4::Translation(0.0f, model.params.wheelBase * 0.5, 0.0f)))
    {}
    
    const Mesh& getMesh() const { return mesh; }

    void speedUp(float dSpeed)
    {
        model.accelerate(dSpeed);
    }

    void speedDown(float faktor)
    {
        model.brake(faktor);
    }

    void steerFrontWheels(float dAngle)
    {
        model.steer(dAngle);
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    void steerFrontWheelsReset(float dt)
    {
        model.steerReset(dt);
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    Mtx4 getCarMatrix() const
    {
        return model.getTransformation();
    }

    Mtx4 getIcrTransformation() const {

        Mtx4 m = Mtx4::Identity();
        m.translate(model.getIcr(), 0.0f, 0.0f);
        return model.getTransformation() * m;
    }

    void run(float dt)
    {
        // fyzika
        model.update(dt);

        Mtx4 object = Mtx4::Identity().translate(model.getPosition().x, model.getPosition().y, model.getPosition().z) * model.getHeading().toMatrix();

        // roll kol
        float dRoll = (model.getSpeed() * dt) / model.params.wheelRadius;

        rollAllWheels(dt);

        // vizuální steer kol
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    void rollAllWheels(float dt)
    {
        float eps = 1e-3f;

        float v = model.getSpeed();
        float icr = model.getIcr();
        float r = model.params.wheelRadius;

        float turnSign = (icr > 0 ? +1.0f : -1.0f);

        float Omega = v / icr;

        // rovná jízda
        if (!std::isfinite(icr)) {
            // rovná jízda
            float w = v / r;
            wheelFrontLeft.doRoll(w * dt);
            wheelFrontRight.doRoll(w * dt);
            wheelBackLeft.doRoll(w * dt);
            wheelBackRight.doRoll(w * dt);
            return;
        }

        // rychlosti kol
        float v_FL = turnSign * Omega * model.getIcrFL();
        float v_FR = turnSign * Omega * model.getIcrFR();
        float v_BL = turnSign * Omega * model.getIcrBL();
        float v_BR = turnSign * Omega * model.getIcrBR();

        // úhlové rychlosti kol
        wheelFrontLeft.doRoll((v_FL / r) * dt);
        wheelFrontRight.doRoll((v_FR / r) * dt);
        wheelBackLeft.doRoll((v_BL / r) * dt);
        wheelBackRight.doRoll((v_BR / r) * dt);
    }

    Mtx4 getFrontLeft()  const { return getCarMatrix() * wheelFrontLeft.get(); }
    Mtx4 getFrontRight() const { return getCarMatrix() * wheelFrontRight.get(); }
    Mtx4 getBackLeft()   const { return getCarMatrix() * wheelBackLeft.get(); }
    Mtx4 getBackRight()  const { return getCarMatrix() * wheelBackRight.get(); }


    friend CarTransformation Slerp(const CarTransformation& a, const CarTransformation& b, float t) {

        CarTransformation interpolated = a;

        interpolated.model.heading = Quaternion::Slerp(a.model.heading, b.model.heading, t);
        interpolated.model.position = Vec4::Lerp(a.model.position, b.model.position, t);
        interpolated.wheelFrontLeft = WheelTransformation::Lerp(a.wheelFrontLeft, b.wheelFrontLeft, t);
        interpolated.wheelFrontRight = WheelTransformation::Lerp(a.wheelFrontRight, b.wheelFrontRight, t);
        interpolated.wheelBackLeft = WheelTransformation::Lerp(a.wheelBackLeft, b.wheelBackLeft, t);
        interpolated.wheelBackRight = WheelTransformation::Lerp(a.wheelBackRight, b.wheelBackRight, t);

        return interpolated;
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

/*
Implementovat nekdy v budoucnu:
	Origin Rebasing
        pri rozsahlem svete a velkym offsetu kamery se zacne projevovat snizujicici se presnost floatu. 
        Resenim je Origin Rebasing, ktery pravidelne posouva svet tak aby kamera byla co nejblize k originu. 
*/

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

    float logicHz = 30;

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

        if (0) {
        
            float num = 0.5f;

            std::cout << GLibpp::Math::reciprocal_debug(num, 8) << std::endl;
            std::cout << 1.0f / num << std::endl;

            exit(0);
        }

        if (false)
        {
			Quaternion q1 = Quaternion::FromEuler(0.0f, GLibpp::Math::deg2rad(45.0f), 0.0f);
            auto q2 = q1;
            q2.rotateX(0.5f);
            
            std::cout << q1.toMatrix() << std::endl;
            std::cout << q2.toMatrix() << std::endl;

            std::cout << Quaternion::Slerp(q1, q2, 0.5f).toMatrix() << std::endl;
            
            exit(0);
        }


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

        if (input.keyboard.isDown(KeyMap::KEY_ENTER)) {
			scene.modelMatrix.rotateY(1.0f * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_SPACE)) {
            scene.modelMatrix.rotateZ(GLibpp::Math::deg2rad(360.0f * dt));

            scene.camera.move((Vec4(0.0f, 4.0f, 0.0f, 1.0f)) * dt);
        }

        bool flag = true;
        if (input.keyboard.isDown(KeyMap::KEY_UP)) {
            scene.car.speedUp(3 * dt);
            flag = false;
        }

        if (input.keyboard.isDown(KeyMap::KEY_DOWN)) {
            scene.car.speedUp(-2 * dt);
            flag = false;
        }

        if (flag) scene.car.speedDown(0.01);


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

        if (input.keyboard.isDown(KeyMap::KEY_CTRL)) {
            //scene.car.model.params.wheelRadius += 0.4 * dt;
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


