#pragma once

#include <utils/image/pixel.hpp>

namespace utils::color
{
	using Color = utils::image::pixel::Pixel;

	inline const Color Black{0,0,0};
	inline const Color White{255,255,255};
	inline const Color Red{255,0,0};
	inline const Color Green{0,255,0};
	inline const Color Blue{0,0,255};
	inline const Color Yellow{255,255,0};
	inline const Color Cyan{0,255,255};	//青色
	inline const Color Purple{255,0,255};	//紫色
	inline const Color LightPink{255,192,203};	//浅粉色
}