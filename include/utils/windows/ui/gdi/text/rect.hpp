#pragma once

#include <windows.h>
#include <utils/windows/ui/gdi/object/select.hpp>
#include <utils/windows/ui/gdi/dc/Handle.hpp>

namespace utils::windows::ui::gdi::text::rect
{
	//获得指定字符串的绘制大小
	inline utils::Index2D get(const std::u8string & text, HFONT font, HWND hwnd)
	{
		//空字符串
		if(text.empty())
			return {};

		//获得hdc
		const auto hdc = GetDC(hwnd);
		utils_assert(hdc);
		utils::windows::ui::gdi::dc::Handle _hdc{hdc, hwnd};

		//使用font
		auto auto_select_old = utils::windows::ui::gdi::object::select::font(hdc, font);

		//准备数据
		const auto text_u16 = utils::text::encoding::utf8_to_utf16(text);

		//查询
		SIZE text_size{};
		utils_assert(GetTextExtentPoint32W(
			hdc, (const wchar_t*)text_u16.c_str(), (int)text_u16.length(), &text_size));

		return {text_size.cx, text_size.cy};
	}
}