#pragma once


#include <vector>
#include "utils/string.hpp"


namespace utils::txt
{
	//移除注释行
	void remove_comment_line(_Inout_ std::vector<std::string> & lines, _In_ const std::string & prefix)
	{
		std::vector<std::string> _lines;
//		_lines.reserve(lines->size());

		for (const auto & line : lines)
		{
			if (utils::string_function::begin_with(line, prefix))
				continue;

			_lines.push_back(line);
		}

		lines = std::move(_lines);
	}
}