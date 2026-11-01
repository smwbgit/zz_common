#pragma once

#include <windows.h>
#include <string>
#include <utils/windows/ui/window/s_hwnd.hpp>
#include <utils/set.hpp>

namespace utils::windows::ui::window::ime
{
	//是否为输入法窗口
	inline bool is_ime_window(HWND hwnd)
	{
		static const std::set<std::u8string> class_names{
			u8"IME",
			u8"MSCTFIME UI",
			u8"SoPY_Status",
			u8"SoPY_Hint",
			u8"SoPY_UI",
			u8"Sogou_TSF_UI",
		};

		const auto class_name = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_class_name();
		return utils::set(class_names).is_exist(class_name);
	}
}