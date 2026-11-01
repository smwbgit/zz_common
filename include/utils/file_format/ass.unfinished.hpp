#pragma once

#include <functional>
#include <filesystem>
#include "utils/string.hpp"
#include "utils/assert.hpp"
#include "utils/FileReader.hpp"

//对ass字幕文件的解析
namespace utils::file_format::ass::unfinished
{
	//遍历所有字幕文字
	//todo: 优化一下临时变量的不必要的使用
	inline void iterator_subtitles(
		_In_ const std::filesystem::path & file,
		_In_ std::function<void(_In_ const std::string & subtitle)> meet_one_subtitle)
	{
		//读取文件
		utils::string text;
		{
			utils::FileReader input{file};
			input.read_whole_file(text);
		}

		//函数: 提取Events
		auto grab_part_events = [](
			_In_ const std::string & text,
			_In_ std::function<void(_In_ const std::string & part)> meet_part)
		{
//			utils::string_view _text{text};
			utils::string _text = text;	//todo: 换为使用上边的那一行
			utils::string part;
			_text.substr_by_string(part, "[Events]\r\n");
					
			//简单检查
			utils_assert(part.exist("\n[") == false);

			meet_part(part);
		};

		grab_part_events(text, [&](const utils::string & part)
		{
			std::vector<std::string> lines;
			part.split(lines, "\r\n");

			//简单检查
			utils_assert(lines.size() >= 2);

			//确定字幕在哪一列
			uint64_t column_index{};
			{
				//简单检查
				utils_assert(lines[0].starts_with("Format:"));

				utils::string content = lines[0].substr(strlen("Format:"));
				content.split(",", [&](const std::vector<std::string> & columns)
				{
					//简单检查
					utils_assert(columns.size() >= 1);

					auto last_one = columns[columns.size()-1];
					utils::string_function::trim(last_one, ' ');

					//简单检查
					utils_assert(last_one == "Text");

					column_index = columns.size()-1;
				});
			}

			for (uint64_t i = 1; i < lines.size(); i++)
			{
				const auto & line = lines[i];

				if (line.empty())
					continue;

				//todo 打开下边的断言
#if 1
				if (line.starts_with("Dialogue:") == false)
					continue;
#else
				//简单检查
				utils_assert(utils::string_function::begin_with(line, "Dialogue:"));
#endif

				utils::string content = line.substr(strlen("Dialogue:"));
				utils::string last_one;
				content.substr_by_nth_string(last_one, ",", column_index);
				utils::string_function::trim(last_one, ' ');

				//给出
				meet_one_subtitle(last_one);
			}
		});
	}
}
