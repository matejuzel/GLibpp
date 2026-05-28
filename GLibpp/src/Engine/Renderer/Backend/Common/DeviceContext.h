#pragma once

#include <cstdint>
#include "Viewport.h"
#include "Mtx4.h"
#include "Color.h"

namespace Render {
	
    template<typename Device>
    class DeviceContext {

    public:
        DeviceContext() = default;
        ~DeviceContext() = default;

        inline void setViewport(Viewport viewport)
        {
            this->viewport = viewport;
        }
        
        inline void setView(const Mtx4& mtx)
        { 
            view = mtx;
            updateViewProjection(); 
        }

        inline void setProjection(const Mtx4& mtx)
        {
            projection = mtx;
            updateViewProjection();
        }


        inline Mtx4 getViewProjection() const
        { 
            return viewProjection; 
        }

        inline Mtx4 getModelView(const Mtx4& model) const
        {
            return view * model;
        }

		inline Mtx4 getModelViewProjection(const Mtx4& model) const 
        {
            return viewProjection * model;
        }
        
        inline Viewport getViewport() const
        {
            return viewport;
        }


    private:

        void updateViewProjection() 
        {
            viewProjection = projection * view;
		}

        Viewport viewport = { 0,0,800,600 };
        Mtx4 view = Mtx4::Identity();
        Mtx4 projection = Mtx4::Identity();

        // spocitane
        Mtx4 viewProjection = Mtx4::Identity();

    public:

        Color clearColor = { 0,0,0,255 };
        uint32_t frameIndex = 0;

        typename Device::TargetHandle framebufferHandle = Device::TARGET_INVALID;
        typename Device::TargetHandle depthbufferHandle = Device::TARGET_INVALID;
    };

}

