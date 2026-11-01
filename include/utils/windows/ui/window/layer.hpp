#pragma once

#include <windows.h>

namespace utils::windows::ui::window::layer
{
	//获得当前窗口
	inline HWND get_foreground_window()
	{
		return GetForegroundWindow();
	}
}