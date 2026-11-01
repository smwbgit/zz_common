#pragma once

#include <windows.h>

namespace utils::windows::ui::window::active_window
{
	//获得系统中的当前激活窗口
	inline HWND get_active_window()
	{
		HWND active_window = GetForegroundWindow();
		return active_window;
	}
}