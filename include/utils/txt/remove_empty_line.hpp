#pragma once

#include <vector>
#include <string>

namespace utils::txt
{
	//移除空行
	void remove_empty_line(_Inout_ std::vector<std::string> & lines)
	{
		std::vector<std::string> _lines;
//		_lines.reserve(lines.size());

		for (const auto & line : lines)
		{
			if (line.empty())
				continue;

			_lines.push_back(line);
		}

		lines = std::move(_lines);
	}
}