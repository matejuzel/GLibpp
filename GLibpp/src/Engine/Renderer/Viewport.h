#pragma once

namespace Render {

	struct Viewport {
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;

		float computeAspectRatio() const noexcept {
			return static_cast<float>(width) / static_cast<float>(height);
		}

		void resize(uint32_t newWidth, uint32_t newHeight) {
			width = newWidth;
			height = newHeight;
		}
	};
}