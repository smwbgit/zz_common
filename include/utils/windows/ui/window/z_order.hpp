#pragma once

#include <windows.h>
#include <optional>
#include <utils/windows/ui/window/s_hwnd.hpp>
#include <utils/windows/ui/screen.hpp>
#include <utils/windows/process/integrity_level.hpp>
#include <utils/windows/ui/window/ime.hpp>
#include <utils/Range2D.hpp>

namespace utils::windows::ui::window::z_order
{
	//获得z序的最前面的可见窗口，跳过topmost窗口和输入法窗口，以及完整性过高的窗口
	//note: 用于获取屏幕上的主要窗口/工作窗口
	inline std::optional<HWND> get_work_window()
	{
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;

		auto current = GetTopWindow(NULL);

		while(1)
		{
			if( [&]()  {
				//跳过完整性过高的窗口
				if( [&]()  {
					const auto pid = s_hwnd(current).get_pid();
					return utils::windows::process::integrity_level::is_too_high(pid);  }())
				{
					return false;
				}

				//跳过topmost窗口
				if(s_hwnd(current).is_always_on_top())
					return false;

				//跳过输入法窗口
				if(utils::windows::ui::window::ime::is_ime_window(current))
					return false;

				//跳过不可见窗口
				if(!s_hwnd(current).is_visible())
					return false;

				//跳过由于x,y坐标设置的过大过小而不可见的窗口
				if([&]()  {
					const auto window_rect = s_hwnd(current).get_rect();
					const auto screen_rect = utils::windows::ui::screen::get_screen_rect();

					return !window_rect.is_overlap(screen_rect);  }())
				{
					return false;
				}

				//跳过窗口大小为0的窗口
				if(s_hwnd(current).get_size() == utils::Index2D{0,0})
					return false;

				return true;  }())
			{
				return current;
			}

			current = GetWindow(current, GW_HWNDNEXT);
			if(!current)
			{
				//简单检查
				utils_assert(ERROR_SUCCESS == GetLastError());

				return {};
			}
		}
	}
}