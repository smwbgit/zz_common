#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows::ui::window
{
	//给一个窗口绑定一个指定类的对象，使其能在类中处理消息
	template<typename T>
	class ObjectBinder
	{
	public:
		//通过窗口句柄获得对象
		static T & get_object(HWND hwnd)
		{
			auto wnd = (T*)GetWindowLongPtrW(hwnd, 0);
			utils_assert(wnd);
			return *wnd;
		}
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			//创建TipWindow对象
			if (!GetWindowLongPtrW(hwnd, 0))
			{
				//简单检查
				utils_assert(uMsg == WM_NCCREATE || uMsg == WM_GETMINMAXINFO);

				SetWindowLongPtrW(hwnd, 0, (LONG_PTR)new T{});
			}

			//转发
			auto return_val = get_object(hwnd).WindowProc(hwnd, uMsg, wParam, lParam);

			//释放TipWindow对象
			if (uMsg == WM_NCDESTROY)
			{
				//简单检查
				utils_assert(GetWindowLongPtrW(hwnd, 0));

				delete &get_object(hwnd);
			}
			return return_val;
		}
	};
}