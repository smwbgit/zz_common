#pragma once

#include <windows.h>
#include <utils/callback/CallbackAdapter.hpp>
#include <utils/assert.hpp>

namespace utils::windows::ui::window::get_information::global
{
	//获得所有顶层窗口
	inline std::vector<HWND> get_all_window()
	{
		std::vector<HWND> all_window;
		utils::callback::CallbackAdapter<1, BOOL, HWND, LPARAM> callback{
			[&all_window](HWND hwnd, LPARAM) ->BOOL
		{
			all_window.push_back(hwnd);
			return TRUE;
		}};
		utils_assert(EnumWindows(callback.callback_stdcall(), callback.param()));
		return all_window;
	}
	//获得拥有焦点的顶级窗口
	inline std::optional<HWND> get_foreground_window()
	{
		HWND hwnd = GetForegroundWindow();

		//当前没有拥有焦点的顶级窗口
		if(hwnd == NULL)
			return {};

		return {hwnd};
	}
}