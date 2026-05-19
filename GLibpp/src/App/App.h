#pragma once
#include <memory>

#include "WindowWin32.h"

#include "Mathematics.h"
#include "Mtx4.h"
#include "Vec4.h"
#include "Mesh.h"
#include "Quaternion.h"



namespace GLibpp::Physics {

    struct BicicleParams {
        BicicleParams(float wheelRadius, float wheelBase, float wheelTrack, float maxSteerAngle) : wheelRadius(wheelRadius), wheelBase(wheelBase), wheelTrack(wheelTrack), maxSteerAngle(maxSteerAngle) {}
         float wheelRadius; // polomer kola
         float wheelBase; // rozvor (vzdalenost os predni a zadni napravy)
         float wheelTrack; // rozchod (vzdalenost kol na jedne naprave)
         float maxSteerAngle;
    };

    class BicycleModel {

    public:
        BicicleParams params;

    private:
        Vec4 position;        // světová pozice
        Quaternion heading;   // NEW – plná 3D orientace

        float speed = 0.0f;
        float steerAngle = 0.0f;

    public:

        BicycleModel(float wheelRadius, float wheelBase, float wheelTrack, float maxSteerAngle)
            : params(wheelRadius, wheelBase, wheelTrack, maxSteerAngle),
            heading(Quaternion::Identity()) // NEW
        {
        }

        static BicycleModel Basic()
        {
            return BicycleModel(0.3f, 2.0f, 1.0f, Math::deg2rad(35.0f));
        }

        float getIcr() const
        {
            return params.wheelBase / tan(steerAngle);
        }

        float getSteerLeft() const
        {
            return atan(params.wheelBase / (getIcr() + params.wheelTrack * 0.5f));
        }

        float getSteerRight() const
        {
            return atan(params.wheelBase / (getIcr() - params.wheelTrack * 0.5f));
        }

        // ---------------------------------------------------------
        //                  UPDATE (Quaternion verze)
        // ---------------------------------------------------------

        void update(float dt)
        {
            // 1) Forward vektor z quaternionu
            Vec4 forward = heading * Vec4(0, 0, 1, 0);

            // 2) Rovná jízda
            if (fabs(steerAngle) < 0.0001f) {
                position.x += forward.x * speed * dt;
                position.z += forward.z * speed * dt;
                return;
            }

            // 3) Poloměr zatáčení
            float R = getIcr();
            float dtheta = (speed * dt) / R;

            // 4) Rotace kolem Y (yaw)
            Quaternion delta = Quaternion::FromAxisAngle(Vec4(0, 1, 0, 0), dtheta);
            heading = (heading * delta).normalized();

            // 5) Nový forward
            forward = heading * Vec4(0, 0, 1, 0);

            // 6) Posun
            position.x += forward.x * speed * dt;
            position.z += forward.z * speed * dt;
        }


        // ---------------------------------------------------------

        void accelerate(float dv)
        {
            speed += dv;
            if (fabs(speed) < 0.01f)
                speed = 0.0f;
        }

        void brake(float faktor)
        {
            if (speed > 0.1f) speed -= faktor;
            if (speed < -0.1f) speed += faktor;
        }

        void steer(float dAngle)
        {
            steerAngle += dAngle;

            if (steerAngle > params.maxSteerAngle)
                steerAngle = params.maxSteerAngle;

            if (steerAngle < -params.maxSteerAngle)
                steerAngle = -params.maxSteerAngle;
        }

        void steerReset(float dt)
        {
            float delta = 4.0f * params.maxSteerAngle * dt;

            if (fabs(steerAngle) <= delta)
                steerAngle = 0.0f;
            else
                steerAngle -= copysign(delta, steerAngle);
        }

        float getSpeed() const { return speed; }

        // NEW – heading je quaternion, takže vracíme quaternion
        Quaternion getHeading() const { return heading; }

        Vec4 getPosition() const { return position; }
    };

};



struct WheelTransformation {

    WheelTransformation(float zPos, float xPos, float radius)
        : position(Vec4(xPos, 0.0f, zPos, 1.0f))
        , mesh(Mesh::Cylinder(radius, 0.16, 12).applyTransformation(Mtx4::RotationZ(3.14159f / 2.0f)))
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

    Mesh getMesh() const { return mesh; }

    float steerAngle = 0.0f; // rad 
    float rollAngle = 0.0f;
private:

    Mesh mesh;
    Vec4 position;

    float steerAngleMax = GLibpp::Math::deg2rad(35.0f);
    
    

};

/*
struct CarTransformation {
    
    float speed = 0.0f; //  m / s
    float wheelRadius = 0.3f; // polomer kola
    float wheelBase = 2.0; // rozvor (vzdalenost os predni a zadni napravy)
    float wheelTrack = 1.0; // rozchod (vzdalenost kol na jedne naprave)

    float getIcr() const {

        if (abs(wheelFrontLeft.steerAngle) < 0.001) return 0.0;

        return wheelBase / tan(wheelFrontLeft.steerAngle);
    }

    Mtx4 getIcrTransformation() const {
       
        Mtx4 m = Mtx4::Identity();
        m.translate(getIcr(), 0.0f, -wheelBase / 2.0f);

        return object * m;
    }

    CarTransformation()
        : object(Mtx4::Identity())
        , wheelFrontLeft ( wheelBase/2.0f, -wheelTrack/2.0f, wheelRadius)
        , wheelFrontRight( wheelBase/2.0f,  wheelTrack/2.0f, wheelRadius)
        , wheelBackLeft  (-wheelBase/2.0f, -wheelTrack/2.0f, wheelRadius)
        , wheelBackRight (-wheelBase/2.0f,  wheelTrack/2.0f, wheelRadius)
        , mesh(Mesh::Cylinder(1.0f, 6, 16).applyTransformation(Mtx4::RotationX(3.14159 / 2.0f)))
    {
    }

    const Mesh& getMesh() const {
        return mesh;
    }

    void speedUp(float dSpeed) 
    {
        speed += dSpeed;

        if (abs(speed) < 0.01) speed = 0.0f;
    }

    void speedDown(float faktor) {

        if (speed > 0.1) speed -= faktor;
        if (speed < -0.1) speed += faktor;
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

        //object.rotateY(wheelFrontLeft.steerAngle * dt * speed); // tohle by bylo kolem stredu auto, ale musime podle stredu zadni napravy...
        if (abs(getIcr()) > 0.00001) {
        
            auto T = Mtx4::Translation(0.0f, 0.0f, -wheelBase / 2.0f);
            auto R = Mtx4::RotationY(dt * speed / getIcr());
            auto Tinv = Mtx4::Translation(0.0f, 0.0f, wheelBase / 2.0f);
            object = object * T * R * Tinv;
        }
        
        object.translate(0.0f, 0.0f, speed * dt);
        rollAllWheels(speed * dt / wheelRadius);
    }

    void rollAllWheels(float dAngle) {
        wheelFrontLeft.doRoll(dAngle);
        wheelFrontRight.doRoll(dAngle);
        wheelBackLeft.doRoll(dAngle);
        wheelBackRight.doRoll(dAngle);
    }

    Mtx4 object;
    Mesh mesh;

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
*/

struct CarTransformation
{
    // fyzikální model – poloha = střed zadní nápravy
    GLibpp::Physics::BicycleModel model;

    // vizuální parametry (API zachováno)
    float speed = 0.0f;
    float wheelRadius = 0.3f;
    float wheelBase = 2.0f;
    float wheelTrack = 1.0f;

    // wheel transformace
    WheelTransformation wheelFrontLeft;
    WheelTransformation wheelFrontRight;
    WheelTransformation wheelBackLeft;
    WheelTransformation wheelBackRight;

    // transformace auta (původ = střed zadní nápravy)
    Mtx4 object;
    Mesh mesh;

    CarTransformation()
        : model(GLibpp::Physics::BicycleModel::Basic())
        , wheelFrontLeft(+wheelBase * 0.5f, -wheelTrack * 0.5f, wheelRadius)
        , wheelFrontRight(+wheelBase * 0.5f, +wheelTrack * 0.5f, wheelRadius)
        , wheelBackLeft(-wheelBase * 0.5f, -wheelTrack * 0.5f, wheelRadius)
        , wheelBackRight(-wheelBase * 0.5f, +wheelTrack * 0.5f, wheelRadius)
        , object(Mtx4::Identity())
        , mesh(Mesh::Cylinder(1.0f, 6, 16).applyTransformation(Mtx4::RotationX(3.14159f / 2.0f)))
    {
    }

    const Mesh& getMesh() const { return mesh; }

    // ------------------------------------------------------------
    // API – delegace do BicycleModel
    // ------------------------------------------------------------

    void speedUp(float dSpeed)
    {
        model.accelerate(dSpeed);
        speed = model.getSpeed();
    }

    void speedDown(float faktor)
    {
        model.brake(faktor);
        speed = model.getSpeed();
    }

    void steerFrontWheels(float dAngle)
    {
        model.steer(dAngle);
        wheelFrontLeft.steerAngle = model.getSteerLeft();
        wheelFrontRight.steerAngle = model.getSteerRight();
    }

    void steerFrontWheelsReset(float dt)
    {
        model.steerReset(dt);
        wheelFrontLeft.steerAngle = model.getSteerLeft();
        wheelFrontRight.steerAngle = model.getSteerRight();
    }

    float getIcr() const
    {
        return model.getIcr();
    }

    // ------------------------------------------------------------
    // Transformace auta – PŮVOD = střed zadní nápravy
    // ------------------------------------------------------------

    Mtx4 getCarMatrix() const
    {
        return Mtx4::Identity()
            .translate(model.getPosition().x, model.getPosition().y, model.getPosition().z) * model.getHeading().toMatrix()
            .translate(0, 0, wheelBase * 0.5f);
    }


    Mtx4 getIcrTransformation() const
    {
        return getCarMatrix();
    }

    // ------------------------------------------------------------
    // UPDATE
    // ------------------------------------------------------------
    void run(float dt)
    {
        // fyzika
        model.update(dt);

        object = Mtx4::Identity().translate(model.getPosition().x, model.getPosition().y, model.getPosition().z) * model.getHeading().toMatrix();

        // roll kol
        float dRoll = (model.getSpeed() * dt) / wheelRadius;
        rollAllWheels(dRoll);

        // vizuální steer kol
        wheelFrontLeft.steerAngle = model.getSteerLeft();
        wheelFrontRight.steerAngle = model.getSteerRight();
    }

    void rollAllWheels(float dAngle)
    {
        wheelFrontLeft.doRoll(dAngle);
        wheelFrontRight.doRoll(dAngle);
        wheelBackLeft.doRoll(dAngle);
        wheelBackRight.doRoll(dAngle);
    }

    // ------------------------------------------------------------
    // TRANSFORMACE KOL – API zachováno
    // ------------------------------------------------------------
    Mtx4 getFrontLeft()  const { return object * wheelFrontLeft.get(); }
    Mtx4 getFrontRight() const { return object * wheelFrontRight.get(); }
    Mtx4 getBackLeft()   const { return object * wheelBackLeft.get(); }
    Mtx4 getBackRight()  const { return object * wheelBackRight.get(); }
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


