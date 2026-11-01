#pragma once

#include <string>

//排版
namespace utils::text::layout
{
	//右对齐，对于单行文本
	inline std::u8string right_align(const std::u8string & text, uint64_t column_width)
	{
		if (text.length() >= column_width)
			return text;
		return std::u8string(column_width - text.length(), u8' ') + text;
	}
	//左对齐，对于单行文本
	inline std::u8string left_align(const std::u8string & text, uint64_t column_width)
	{
		if (text.length() >= column_width)
			return text;
		return text + std::u8string(column_width - text.length(), u8' ');
	}
}