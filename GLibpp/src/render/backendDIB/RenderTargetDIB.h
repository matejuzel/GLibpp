#pragma once

#include "IRenderTarget.h"

class RenderTargetDIB : public IRenderTarget
{
	private:
	uint32_t width_;
	uint32_t height_;

public:

	RenderTargetDIB(uint32_t width, uint32_t height) : width_(width), height_(height) {}
	int width() const override { return width_; }
	int height() const override { return height_; }

};