#pragma once

#include <vector>
#include <windows.h>
#include <utils/assert.hpp>
#include <utils/Index2D.hpp>
#include <utils/windows/ui/screen.hpp>

namespace utils::windows::ui::window::size
{
	//是否屏幕显示不下
	inline bool is_bigger_than_workarea(utils::Index2D size)
	{
		auto workarea_size = utils::windows::ui::screen::get_workarea_size();

		return size.i > workarea_size.i || size.j > workarea_size.j;
	}
}