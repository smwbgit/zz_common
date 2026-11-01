#pragma once

#include "utils/string.hpp"
#include "utils/make.hpp"
#include "utils/text/hex.hpp"

//html的实体编码: 实体字符和实体编码
namespace utils::data_encoding::html::entity
{
	//进行编码
	//void encode(_Out_ std::vector<uint8_t> & to, _In_ const std::vector<uint8_t> & from)
	//{
	//}

	//进行解码
	std::string decode(_In_ const std::string & text)
	{
		utils::string _text = text;
		_text.replace_all(std::regex{"&[^;]+;"}, [](const std::string & fragment)->std::string
		{
			//refer: https://blog.csdn.net/jiaguoxinzhi/article/details/45313279

			if(fragment == "&lt;") return "<";
			if(fragment == "&gt;") return ">";
			if(fragment == "&amp;") return "&";
			if(fragment.length() == 8 &&
				fragment[0] == '&' &&
				fragment[1] == '#' &&
				fragment[2] == 'x' &&
				isxdigit(fragment[3]) &&
				isxdigit(fragment[4]) &&
				isxdigit(fragment[5]) &&
				isxdigit(fragment[6]) &&
				fragment[7] == ';')
			{
				char number[5] = {fragment[5], fragment[6], fragment[3], fragment[4], 0};
				auto unicode = utils::make<std::wstring>(utils::text::hex::from_text(number, false));
				auto utf8 = utils::character_encoding::convert_unicode_to_utf8(unicode);
				return utf8;
			}
			if(fragment.length() == 5 &&
				fragment[0] == '&' &&
				fragment[1] == '#' &&
				isdigit(fragment[2]) &&
				isdigit(fragment[3]) &&
				fragment[4] == ';')
			{
				char number[3] = {fragment[2], fragment[3], 0};
				int _number{};
				sscanf(number, "%d", &_number);
				return std::string() + (char)_number;
			}

			utils_throw();
		});
		return _text;
	}
}