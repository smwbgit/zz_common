#pragma once

#include <windows.h>
#include "utils/assert.hpp"

//未整理
namespace utils::windows::ui::message
{
	namespace inner::update_window_recursively
	{
		inline BOOL meet_one_child_window(HWND hwnd, LPARAM);
	}

	//递归更新所有窗口
	inline void update_window_recursively(_In_ HWND hwnd)
	{
		utils_assert(UpdateWindow(hwnd));

		EnumChildWindows(hwnd, inner::update_window_recursively::meet_one_child_window, 0);
	}

	namespace inner::update_window_recursively
	{
		inline BOOL meet_one_child_window(HWND hwnd, LPARAM)
		{
			utils::windows::ui::message::update_window_recursively(hwnd);
			return TRUE;
		}
	}
}