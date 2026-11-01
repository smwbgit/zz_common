#pragma once


#include <string>
#include "utils/assert.hpp"


namespace utils::txt
{
	// 格式化表格中的一个单元格: 扩展一个字符串到指定长度
	inline void format_table_cell(std::string & text, uint64_t column_width)
	{
		utils_assert(text.length() <= column_width);

		text += std::string(column_width - text.length(), ' ');
	}
}