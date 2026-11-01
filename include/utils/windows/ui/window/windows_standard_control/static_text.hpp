#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/integer_type.hpp>
#include <utils/windows/ui/window/s_hwnd.hpp>

namespace utils::windows::ui::window::windows_standard_control::static_text
{
	//文本对齐方式
	//note: 类似 utils::ui::layout::layout_manager::LayoutAlignValue
	enum class TextAlignValue
	{
		Small,	//向小方向对齐, 比如左边，上边
		Middle,	//向中间对齐
		Large,	//向大方向对齐, 比如右边，下边
	};

	//设置文本对齐
	inline void set_text_align(HWND hwnd, TextAlignValue align)
	{
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;

		switch(align)
		{
		case TextAlignValue::Small:
			s_hwnd(hwnd).modify_style(SS_LEFT, SS_CENTER | SS_RIGHT);
			return;
		case TextAlignValue::Middle:
			s_hwnd(hwnd).modify_style(SS_CENTER, SS_LEFT | SS_RIGHT);
			return;
		case TextAlignValue::Large:
			s_hwnd(hwnd).modify_style(SS_RIGHT, SS_LEFT | SS_CENTER);
			return;
		}
		utils_throw();
	}
}