#pragma once

class IRenderTarget
{
public:

	virtual int width() const = 0;
	virtual int height() const = 0;
	virtual ~IRenderTarget() = default;

};