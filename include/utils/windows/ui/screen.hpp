#pragma once

#include <windows.h>
#include <utils/Index2D.hpp>
#include <utils/Range2D.hpp>

namespace utils::windows::ui::screen
{
	//获得屏幕大小
	inline utils::Index2D get_screen_size()
	{
		uint64_t width  = GetSystemMetrics(SM_CXSCREEN); utils_assert(width  > 0);
		uint64_t height = GetSystemMetrics(SM_CYSCREEN); utils_assert(height > 0);

		return {width, height};
	}
	//特化
	inline utils::Range2D get_screen_rect()
	{
		return { {0,0}, get_screen_size() };
	}
	//获得工作区（即屏幕上去掉任务栏和其他停靠窗口后的区域）大小
	inline utils::Index2D get_workarea_size()
	{
		RECT rect{};
		utils_assert(SystemParametersInfoA(SPI_GETWORKAREA, 0, &rect, 0));

        return {
			rect.right - rect.left,
			rect.bottom - rect.top
		};
	}
}