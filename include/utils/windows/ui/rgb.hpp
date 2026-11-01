#pragma once

#include <windows.h>
#include <utils/color/Color.hpp>

namespace utils::windows::ui::rgb
{
	inline COLORREF from_color(utils::color::Color color)
	{
		return RGB(color.r, color.g, color.b);
	}
	inline utils::color::Color to_color(COLORREF rgb)
	{
		return {GetRValue(rgb), GetGValue(rgb), GetBValue(rgb)};
	}
}