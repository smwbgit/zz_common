#pragma once

#include <windows.h>
#include <utils/Index2D.hpp>

namespace utils::windows::ui::mouse
{
	//鼠标当前位置
	inline utils::Index2D get_position()
	{
		POINT pos{};
		utils_assert(GetCursorPos(&pos));
		return {pos.x, pos.y};
	}
	//设置鼠标位置
	inline void set_position(utils::Index2D position)
	{
		utils_assert(SetCursorPos(position.i, position.j));
	}
}