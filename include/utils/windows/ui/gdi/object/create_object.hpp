#pragma once

#include <windows.h>
#include <utils/windows/ui/gdi/object/Object.hpp>
#include <utils/basic/store_this_integer.hpp>
#include <utils/integer_type.hpp>

namespace utils::windows::ui::gdi::object
{
#pragma region 画笔
	//画笔
	using Pen = Object<HPEN>;

	//获取画笔
	inline Pen create_pen(const int style, const s8 width, const COLORREF color)
	{
		//数据宽度转换
		const auto width_s4 = utils::basic::store_in<s4>(width);

		//创建画笔
		HPEN hPen = ::CreatePen(style, width_s4, color);

		//简单检查
		utils_assert(hPen);

		//给出
		return Pen{hPen};
	}
	//特化
	inline Pen create_pen(const int style, const s8 width, const utils::color::Color color)
	{
		//转为windows表示
		const COLORREF _color = RGB(color.r, color.g, color.b);

		//转发
		return create_pen(style, width, _color);
	}
#pragma endregion

#pragma region 画刷
	//画刷
	using Brush = Object<HBRUSH>;

	//获取画刷
	inline Brush create_brush(const COLORREF color)
	{
		//创建画刷
		HBRUSH hBrush = ::CreateSolidBrush(color);

		//简单检查
		utils_assert(hBrush);

		//给出
		return Brush{hBrush};
	}
	//特化
	inline Brush create_brush(const utils::color::Color color)
	{
		//转为windows表示
		const COLORREF _color = RGB(color.r, color.g, color.b);

		//转发
		return create_brush(_color);
	}
#pragma endregion

#pragma region 字体
	// 请使用 utils::windows::ui::gdi::object::font 里的相关函数
#pragma endregion
}