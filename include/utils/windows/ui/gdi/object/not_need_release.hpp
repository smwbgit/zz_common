#pragma once

#include <windows.h>
#include <utils/map.hpp>
#include <utils/color/Color.hpp>
#include <utils/windows/ui/rgb.hpp>
#include <utils/windows/ui/gdi/object/create_object.hpp>
#include <utils/windows/ui/gdi/object/font.hpp>

//全局缓存gdi的对象，重复使用，不再释放，直到程序结束
namespace utils::windows::ui::gdi::object::not_need_release
{
	inline HBRUSH get_brush(COLORREF color)
	{
		static std::map<COLORREF, HBRUSH> brush_table;

		//创建对象
		if(!utils::map(brush_table).is_exist(color))
		{
			HBRUSH brush = ::CreateSolidBrush(color);
			utils_assert(brush);

			utils::map(brush_table).insert_not_exist(color, brush);
		}

		//给出
		return utils::map(brush_table).get_existed_value(color);
	}
	//特化
	inline HBRUSH get_brush(utils::color::Color color)
	{
		const auto rgb = utils::windows::ui::rgb::from_color(color);

		//转发
		return get_brush(rgb);
	}
	inline HFONT get_font(const std::u8string & font_name, uint64_t font_size)
	{
		// 准备声明
		using Font = utils::windows::ui::gdi::object::font::Font;

		// 准备数据
		static std::map<std::u8string, std::map<uint64_t, Font>> font_table;

		// 还没有此字体
		if(!utils::map(font_table).is_exist(font_name) || !utils::map(font_table[font_name]).is_exist(font_size))
		{
			// 创建字体
			auto font = utils::windows::ui::gdi::object::font::create_font(font_name, font_size);

			// 保存
			font_table[font_name][font_size] = std::move(font);
		}

		// 给出
		return font_table[font_name][font_size];
	}
}