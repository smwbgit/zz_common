#pragma once

#include <string>
#include <utils/data.hpp>
#include <utils/text/combine.hpp>

//文本::十六进制
namespace utils::text::hex
{
	//十六进制字符串 转为 字节数组
	//note:十六进制字符串里大小写都支持
	//e.g.    "12AB34"  ->  std::vector<uint8_t>{0x12, 0xAB, 0x34}
	//e.g.    "12ab34"  ->  std::vector<uint8_t>{0x12, 0xAB, 0x34}
	//is_can_contain_space: 是否可以包含空格,\n,\t
	inline utils::data from_text(_In_ const std::u8string & text, bool is_can_contain_space = false)
	{
		utils::data data;

		for (std::size_t i = 0; ; )
		{
			if(i == text.length()) break;
			
			if(text[i] == u8' ' || text[i] == u8'\n' || text[i] == u8'\t')
			{
				//简单检查
				utils_assert(is_can_contain_space);

				i++; continue;
			}

			//简单检查
			utils_assert(i + 1 < text.length());
			
			char _text[3] = {(char)text[i], (char)text[i + 1], 0};
			uint8_t byte{};
			utils_assert(1 == sscanf(_text, "%02hhX", &byte));
			data.append(byte);
			i+=2;
		}
		return data;
	}
	//特化
	inline void from_text(_Out_ utils::data & data, _In_ const std::u8string & text)
	{
		data = from_text(text);
	}
	//特化
	inline void from_text(_Out_ uint8_t & byte, _In_ const std::u8string & text)
	{
		utils::data data = from_text(text, false);
		utils_assert(data.size() == 1);
		byte = data[0];
	}
	//字节数组 转为 十六进制字符串
	//e.g.   std::vector<uint8_t>{0x12, 0xab}   ->    u8"12AB"
	inline std::u8string to_text(_In_ utils::data_view data, _In_ bool is_upper_case = true, _In_ const std::u8string & separator = u8"", _In_ const std::u8string & prefix = u8"", _In_ const std::u8string & postfix = u8"")
	{
		static const char8_t hex_table_upper[256][3] =
		{
			u8"00",u8"01",u8"02",u8"03",u8"04",u8"05",u8"06",u8"07",u8"08",u8"09",u8"0A",u8"0B",u8"0C",u8"0D",u8"0E",u8"0F",
			u8"10",u8"11",u8"12",u8"13",u8"14",u8"15",u8"16",u8"17",u8"18",u8"19",u8"1A",u8"1B",u8"1C",u8"1D",u8"1E",u8"1F",
			u8"20",u8"21",u8"22",u8"23",u8"24",u8"25",u8"26",u8"27",u8"28",u8"29",u8"2A",u8"2B",u8"2C",u8"2D",u8"2E",u8"2F",
			u8"30",u8"31",u8"32",u8"33",u8"34",u8"35",u8"36",u8"37",u8"38",u8"39",u8"3A",u8"3B",u8"3C",u8"3D",u8"3E",u8"3F",
			u8"40",u8"41",u8"42",u8"43",u8"44",u8"45",u8"46",u8"47",u8"48",u8"49",u8"4A",u8"4B",u8"4C",u8"4D",u8"4E",u8"4F",
			u8"50",u8"51",u8"52",u8"53",u8"54",u8"55",u8"56",u8"57",u8"58",u8"59",u8"5A",u8"5B",u8"5C",u8"5D",u8"5E",u8"5F",
			u8"60",u8"61",u8"62",u8"63",u8"64",u8"65",u8"66",u8"67",u8"68",u8"69",u8"6A",u8"6B",u8"6C",u8"6D",u8"6E",u8"6F",
			u8"70",u8"71",u8"72",u8"73",u8"74",u8"75",u8"76",u8"77",u8"78",u8"79",u8"7A",u8"7B",u8"7C",u8"7D",u8"7E",u8"7F",
			u8"80",u8"81",u8"82",u8"83",u8"84",u8"85",u8"86",u8"87",u8"88",u8"89",u8"8A",u8"8B",u8"8C",u8"8D",u8"8E",u8"8F",
			u8"90",u8"91",u8"92",u8"93",u8"94",u8"95",u8"96",u8"97",u8"98",u8"99",u8"9A",u8"9B",u8"9C",u8"9D",u8"9E",u8"9F",
			u8"A0",u8"A1",u8"A2",u8"A3",u8"A4",u8"A5",u8"A6",u8"A7",u8"A8",u8"A9",u8"AA",u8"AB",u8"AC",u8"AD",u8"AE",u8"AF",
			u8"B0",u8"B1",u8"B2",u8"B3",u8"B4",u8"B5",u8"B6",u8"B7",u8"B8",u8"B9",u8"BA",u8"BB",u8"BC",u8"BD",u8"BE",u8"BF",
			u8"C0",u8"C1",u8"C2",u8"C3",u8"C4",u8"C5",u8"C6",u8"C7",u8"C8",u8"C9",u8"CA",u8"CB",u8"CC",u8"CD",u8"CE",u8"CF",
			u8"D0",u8"D1",u8"D2",u8"D3",u8"D4",u8"D5",u8"D6",u8"D7",u8"D8",u8"D9",u8"DA",u8"DB",u8"DC",u8"DD",u8"DE",u8"DF",
			u8"E0",u8"E1",u8"E2",u8"E3",u8"E4",u8"E5",u8"E6",u8"E7",u8"E8",u8"E9",u8"EA",u8"EB",u8"EC",u8"ED",u8"EE",u8"EF",
			u8"F0",u8"F1",u8"F2",u8"F3",u8"F4",u8"F5",u8"F6",u8"F7",u8"F8",u8"F9",u8"FA",u8"FB",u8"FC",u8"FD",u8"FE",u8"FF",
		};
		static const char8_t hex_table_lower[256][3] =
		{
			u8"00",u8"01",u8"02",u8"03",u8"04",u8"05",u8"06",u8"07",u8"08",u8"09",u8"0a",u8"0b",u8"0c",u8"0d",u8"0e",u8"0f",
			u8"10",u8"11",u8"12",u8"13",u8"14",u8"15",u8"16",u8"17",u8"18",u8"19",u8"1a",u8"1b",u8"1c",u8"1d",u8"1e",u8"1f",
			u8"20",u8"21",u8"22",u8"23",u8"24",u8"25",u8"26",u8"27",u8"28",u8"29",u8"2a",u8"2b",u8"2c",u8"2d",u8"2e",u8"2f",
			u8"30",u8"31",u8"32",u8"33",u8"34",u8"35",u8"36",u8"37",u8"38",u8"39",u8"3a",u8"3b",u8"3c",u8"3d",u8"3e",u8"3f",
			u8"40",u8"41",u8"42",u8"43",u8"44",u8"45",u8"46",u8"47",u8"48",u8"49",u8"4a",u8"4b",u8"4c",u8"4d",u8"4e",u8"4f",
			u8"50",u8"51",u8"52",u8"53",u8"54",u8"55",u8"56",u8"57",u8"58",u8"59",u8"5a",u8"5b",u8"5c",u8"5d",u8"5e",u8"5f",
			u8"60",u8"61",u8"62",u8"63",u8"64",u8"65",u8"66",u8"67",u8"68",u8"69",u8"6a",u8"6b",u8"6c",u8"6d",u8"6e",u8"6f",
			u8"70",u8"71",u8"72",u8"73",u8"74",u8"75",u8"76",u8"77",u8"78",u8"79",u8"7a",u8"7b",u8"7c",u8"7d",u8"7e",u8"7f",
			u8"80",u8"81",u8"82",u8"83",u8"84",u8"85",u8"86",u8"87",u8"88",u8"89",u8"8a",u8"8b",u8"8c",u8"8d",u8"8e",u8"8f",
			u8"90",u8"91",u8"92",u8"93",u8"94",u8"95",u8"96",u8"97",u8"98",u8"99",u8"9a",u8"9b",u8"9c",u8"9d",u8"9e",u8"9f",
			u8"a0",u8"a1",u8"a2",u8"a3",u8"a4",u8"a5",u8"a6",u8"a7",u8"a8",u8"a9",u8"aa",u8"ab",u8"ac",u8"ad",u8"ae",u8"af",
			u8"b0",u8"b1",u8"b2",u8"b3",u8"b4",u8"b5",u8"b6",u8"b7",u8"b8",u8"b9",u8"ba",u8"bb",u8"bc",u8"bd",u8"be",u8"bf",
			u8"c0",u8"c1",u8"c2",u8"c3",u8"c4",u8"c5",u8"c6",u8"c7",u8"c8",u8"c9",u8"ca",u8"cb",u8"cc",u8"cd",u8"ce",u8"cf",
			u8"d0",u8"d1",u8"d2",u8"d3",u8"d4",u8"d5",u8"d6",u8"d7",u8"d8",u8"d9",u8"da",u8"db",u8"dc",u8"dd",u8"de",u8"df",
			u8"e0",u8"e1",u8"e2",u8"e3",u8"e4",u8"e5",u8"e6",u8"e7",u8"e8",u8"e9",u8"ea",u8"eb",u8"ec",u8"ed",u8"ee",u8"ef",
			u8"f0",u8"f1",u8"f2",u8"f3",u8"f4",u8"f5",u8"f6",u8"f7",u8"f8",u8"f9",u8"fa",u8"fb",u8"fc",u8"fd",u8"fe",u8"ff",
		};

		//准备数据
		auto hex_table = is_upper_case ? hex_table_upper : hex_table_lower;

		//准备数据
		utils::text::combine::Combiner combiner;
		combiner.m_params.optional.separator = separator;
		combiner.m_params.optional.prefix = prefix;
		combiner.m_params.optional.postfix = postfix;

		//合并
		for (const auto b : data)
		{
			combiner.add(hex_table[b]);
		}

		//给出
		return combiner.get_text();
	}
	//特化
	inline void to_text(_Out_ std::u8string & text, _In_ utils::data_view data)
	{
		text = to_text(data);
	}
}

//简便写法: 十六进制转字节数组
#define utils_hex_to_bytes(A) (utils::text::hex::from_text((A), false))