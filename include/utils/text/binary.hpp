#pragma once

#include <string>
#include <utils/data.hpp>

//文本::二进制
namespace utils::text::binary
{
	//字节 转为 二进制字符串
	//e.g.   0x12   ->    u8"00010010"
	inline std::u8string to_text(uint8_t byte)
	{
		std::u8string text;
		for (int i = 7; i >= 0; i--)
		{
			if (byte & (1 << i))
				text += u8'1';
			else
				text += u8'0';
		}
		return text;
	}
	//特化
	//e.g.   std::vector<uint8_t>{0x12, 0xab}   ->    u8"0001001010101011"
	inline std::u8string to_text(_In_ utils::data_view data)
	{
		std::u8string text;
		for (const auto b : data)
		{
			text += to_text(b);
		}
		return text;
	}
}