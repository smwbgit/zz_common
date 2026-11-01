#pragma once

#include <utils/data.hpp>
#include <utils/map.hpp>
#include <utils/text/常用字.hpp>

namespace utils::binary::data::text::提取
{
	inline std::vector<std::string> GB2312(const utils::data & data)
	{
		std::vector<std::string> texts;
		std::string text;
		std::string one_char;
		uint64_t state{};
		auto meet_断点 = [&]()
		{
			state = 0;
			one_char.clear();
			if (text.length() > 5)
			{
				texts.push_back(text);
			}
			text.clear();
		};
		auto is_常用字 = [&](const std::string & one_char) -> bool
		{
			auto u16 = utils::text::encoding::code_page::to_unicode(one_char, utils::text::encoding::code_page::CodePage::cp936);
			auto u8 = utils::text::encoding::unicode::utf16_to_utf8(u16);
			return utils::text::常用字::chinese(u8) || utils::text::常用字::ascii(u8);
		};
		auto meet_one_char = [&]()
		{
			if(is_常用字(one_char))
				text += one_char;
			else
				meet_断点();
			one_char.clear();
		};
		auto count_1 = [](const uint8_t c)
		{
			if (!(c & 0x80u))	return 0;	//0xxxxxxx
			return 1;	//1xxxxxxx
		};
		for (const auto c : data.wrapped_type())
		{
			auto n = count_1(c);
			if(n == 0)
			{
				if (state != 0) meet_断点();

				one_char += (char)c;
				meet_one_char();
			}
			else
			{
				if (state == 0)
				{
					state = 1; one_char += (char)c;
				}
				else
				{
					state = 0; one_char += (char)c;
					meet_one_char();
				}
			}
		}
		meet_断点();
		return texts;
	}
	inline std::vector<std::u8string> u8(const utils::data & data)
	{
		std::vector<std::u8string> texts;
		std::u8string text;
		std::u8string one_char;
		uint64_t state{};
		auto meet_断点 = [&]()
		{
			state = 0;
			one_char.clear();
			if (text.length() > 5)
			{
				texts.push_back(text);
			}
			text.clear();
		};
		auto meet_one_char = [&](std::u8string _one_char)
		{
			if(utils::text::常用字::chinese(_one_char) || utils::text::常用字::ascii(_one_char))
				text += _one_char;
			else
				meet_断点();
			one_char.clear();
		};
		auto count_1 = [](const uint8_t c)
		{
			if (!(c & 0x80u))	return 0;	//0xxxxxxx
			if (!(c & 0x40u))	return 1;	//10xxxxxx
			if (!(c & 0x20u))	return 2;	//110xxxxx
			if (!(c & 0x10u))	return 3;	//1110xxxx
			if (!(c & 0x08u))	return 4;	//11110xxx
			return 5;	//doesn't care
		};
		for (const auto c : data.wrapped_type())
		{
			auto n = count_1(c);
			if (n == 0)
			{
				if (state != 0) meet_断点();

				meet_one_char({(char8_t)c});
			}
			else if (n == 1)
			{
				if (state == 0)
				{
					meet_断点();
				}
				else
				{
					state--; one_char += (char8_t)c;
					if (state == 0)
					{
						meet_one_char(one_char);
					}
				}
			}
			else if (n == 2 || n == 3 || n == 4)
			{
				if (state != 0)
				{
					meet_断点();
				}
				state = n - 1;
				one_char.clear(); one_char += (char8_t)c;
			}
			else
			{
				meet_断点();
			}
		}
		meet_断点();
		return texts;
	}
	inline std::vector<std::u16string> u16(const utils::data & data)
	{
		std::vector<std::u16string> texts;

		utils::map<char16_t, bool> m_table;
		auto is_ok = [&](const char16_t c)
		{
			if (!m_table.is_exist(c))
			{
				auto _c = utils::text::encoding::unicode::utf16_to_utf8({c});
				m_table[c] = utils::text::常用字::chinese(_c) || utils::text::常用字::ascii(_c);
			}
					
			return m_table.get_existed_value(c);
		};
		auto look_this = [&](uint64_t begin)
		{
			std::u16string text;
			for(uint64_t i = begin; i+1 < data.size(); i+=2)
			{
				auto c = *(char16_t*)(data.m_data.data() + i);
				if(is_ok(c))
				{
					text += c;
				}
				else
				{
					if(text.length() > 2)
					{
						texts.push_back(text);
					}
					text.clear();
				}
			}
			if(text.length() > 2)
			{
				texts.push_back(text);
			}
		};
		look_this(0);
		look_this(1);
		return texts;
	}
}