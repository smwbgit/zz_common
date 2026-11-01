#pragma once

#include <vector>
#include <array>
#include "utils/DataBlockReader.hpp"
#include "utils/string.hpp"

//url编码
namespace utils::data_encoding::url
{
	//进行编码
	void encode(_Out_ std::vector<uint8_t> & to, _In_ const std::vector<uint8_t> & from)
	{
		// 是否需要转换
		auto need_conv = [](const uint8_t c)
		{
			//todo 确定可不可以把所有非数字字母都变成%形式
			if (isalnum(c))
				return false;
			else
				return true;
		};

		to.clear();
		for (const auto c : from)
		{
			if (need_conv(c))
			{
				to.push_back('%');
				std::string _hex;
				utils::string_function::make_hex_string(_hex, (uint8_t)c);
				to.push_back(_hex[0]);
				to.push_back(_hex[1]);
			}
			else
				to.push_back(c);
		}
	}
	//特化
	void encode(_Out_ std::string & to, _In_ const std::string & from)
	{
		std::vector<uint8_t> _from{from.begin(), from.end()};	//todo: remove _from
		std::vector<uint8_t> _to;	//todo: remove _to
		encode(_to, _from);
		to.assign(_to.begin(), _to.end());
	}
	//进行解码
	void decode(_Out_ std::vector<uint8_t> & to, _In_ const std::vector<uint8_t> & from)
	{
		utils::DataBlockReader input{from};
		to.clear();
		while (1)
		{
			if (input.is_readed_all())
				return;

			char c{};
			input.read(c);
			if (c == '%')
			{
				//读两个字符并转换

				std::array<uint8_t, 2> _from{};
				input.read(_from);
			
				std::array<uint8_t, 1> _to{};
				utils::string_function::get_byte_array_from_hex_string(_to, _from);
				to.push_back(_to[0]);
			}
			else
			{
				//直接保存
				to.push_back(c);
			}
		}
	}
	//特化
	void decode(_Out_ std::vector<uint8_t> & to, _In_ const std::string & from)
	{
		std::vector<uint8_t> _from;	//todo:去掉这个临时变量
		_from.assign(from.begin(), from.end());

		return decode(to, _from);
	}
}