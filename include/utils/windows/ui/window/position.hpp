#pragma once

#include <windows.h>
#include <stdint.h>

namespace utils::windows::window::position
{
	//居中窗口
	inline void center_window(HWND hwnd)
	{
		// 获取屏幕的宽度和高度
		int64_t screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int64_t screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// 获取窗口的宽度和高度
		RECT windowRect;
		GetWindowRect(hwnd, &windowRect);
		int64_t windowWidth = windowRect.right - windowRect.left;
		int64_t windowHeight = windowRect.bottom - windowRect.top;

		// 计算窗口的位置，使其居中
		int64_t x = (screenWidth - windowWidth) / 2;
		int64_t y = (screenHeight - windowHeight) / 2;

		// 如果计算出的位置会导致窗口部分在屏幕外，则进行调整
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		// 移动窗口到计算出的位置
		MoveWindow(hwnd, x, y, windowWidth, windowHeight, TRUE);
	}
}