#pragma once


#include <vector>
#include "utils/string.hpp"
#include "utils/new_line_character.hpp"


namespace utils::txt
{
	inline void get_lines(std::vector<std::string> & lines, const std::string & text, const std::string & new_line_character = utils::new_line_character::_n)
	{
		utils::string_function::split(lines, text, new_line_character);

		//去掉最后的空行
		if (lines.size() > 1)
		{
			if (lines[lines.size() - 1].empty())
				lines.erase(lines.end() - 1);
		}
	}
}