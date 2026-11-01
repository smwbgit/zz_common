#pragma once

#include <filesystem>
#include <utils/text/split.hpp>
#include <utils/text/encoding/language/chinese.hpp>
#include <utils/text/encoding.hpp>
#include <utils/FileReader.hpp>

namespace utils::text::small_tool::analysis
{
	//获得其中的中文
	inline std::vector<std::u8string> get_chinese_words(const std::u8string & text)
	{
		std::vector<std::u8string> chinese_words;
		{
			//准备数据
			const auto text_u16 = utils::text::encoding::unicode::utf8_to_utf16(text, (char16_t)1);
			std::vector<std::u16string> chinese_words_2;

			//提取汉语部分
			utils::text::split::RealImpl<std::u16string> split{text_u16};
			split.set_result_array(chinese_words_2);
			split.m_is_keep_empty = false;
			split.set_separator_char([](char16_t c)
			{
				return !utils::text::encoding::language::chinese::is_chinese_char(c);
			});
			split.execute();

			for(const auto & one : chinese_words_2)
			{
				chinese_words.push_back(utils::text::encoding::unicode::utf16_to_utf8(one));
			}
		}
		return chinese_words;
	}
	//特化: 在指定的目录中，递归遍历所有文件，对每个文件，生成一份所含汉字的列表
	inline void get_chinese_words_in_recursive_directory(std::u8string & text_output, const std::u8string & dirname)
	{
		struct s_entry
		{
			std::u8string filename;
			std::vector<std::u8string> chinese_words;
			bool is_seem_not_characters;	//疑似乱码
		};
		std::vector<s_entry> entries;
		{
			for(const auto & file : std::filesystem::recursive_directory_iterator(dirname))
			{
				if(file.is_regular_file())
				{
					utils::FileReader input{file.path()};
					auto text = input.text_u8.read_all();

					auto chinese_words = utils::text::small_tool::analysis::get_chinese_words(text);

					if(!chinese_words.empty())
					{
						//准备数据
						bool is_seem_not_characters = [&chinese_words]()
						{
							utils::u8 count{};
							for(const auto & word : chinese_words)
							{
								if(word.size() == 3)	//1个汉字占3个字节
									count++;
							}

							if (chinese_words.size() < 10)
								return count == chinese_words.size();
							else
								return 1.*count/chinese_words.size() >= 0.95;	//大部分都是1个汉字
						}();

						entries.push_back({file.path().u8string(), chinese_words, is_seem_not_characters});
					}
				}
			}
		}

		//排序
		std::sort(entries.begin(), entries.end(), [](const auto & a, const auto & b)
		{
			if (a.is_seem_not_characters && b.is_seem_not_characters)
				return a.chinese_words.size() < b.chinese_words.size();
			if (a.is_seem_not_characters)
				return false;
			if (b.is_seem_not_characters)
				return true;
			return a.chinese_words.size() > b.chinese_words.size();
		});

		//给出
		for(const auto & entry : entries)
		{
			text_output += entry.filename;
			text_output += u8"\n";

			if(entry.is_seem_not_characters)
			{
				text_output += u8"\t";
				for(const auto & word : entry.chinese_words)
				{
					text_output += word;
					text_output += u8" ";
				}
				text_output += u8"\n";
			}
			else
			{
				for(const auto & word : entry.chinese_words)
				{
					text_output += u8"\t";
					text_output += word;
					text_output += u8"\n";
				}
			}
		}
	}
}