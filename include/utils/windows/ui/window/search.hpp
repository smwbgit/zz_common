#pragma once

#include <vector>
#include <optional>
#include <string>
#include <windows.h>
#include <utils/windows/ui/window/get_information/global.hpp>
#include <utils/windows/ui/window/s_hwnd.hpp>

namespace utils::windows::ui::window::search
{
	inline std::optional<HWND> by_title(const std::u8string & title)
	{
		const auto all_window = utils::windows::ui::window::get_information::global::get_all_window();

		std::vector<HWND> result;
		for(const auto hwnd : all_window)
		{
			const auto _title = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_title();
			if(_title == title)
				result.push_back(hwnd);
		}

		//给出
		if(result.empty()) return {};
		utils_assert(result.size() == 1);
		return {result[0]};
	}
}