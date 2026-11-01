#pragma once

//todo 把这个文件放到utils/file_format/里

#include <vector>
#include <string>
#include "utils/map.hpp"
#include "utils/character_encoding.hpp"
#include "utils/data.hpp"
#include "utils/FileReader.hpp"
#include "utils/FileWriter.hpp"

namespace utils::data_encoding::text_file
{
	//编码
	enum class Encoding
	{
		cp936,	//ansi
		utf16le_no_bom,
		utf16le_bom,
		utf8_no_bom,
		utf8_bom,
	};

	//enum -> std::string
	inline const std::string & to_string(_In_ const Encoding encoding)
	{
		static const utils::map<Encoding, std::string> table = {
			{Encoding::cp936, "cp936"},
			{Encoding::utf16le_no_bom, "utf16le_no_bom"},
			{Encoding::utf16le_bom, "utf16le_bom"},
			{Encoding::utf8_no_bom, "utf8_no_bom"},
			{Encoding::utf8_bom, "utf8_bom"},
		};

		return table.get_existed_value(encoding);
	}
	
	namespace inner
	{
		struct s_entry
		{
			Encoding encoding;
			utils::character_encoding::Encoding character_encoding;
			utils::data bom;
		};

		inline utils::map<Encoding, s_entry> table = {
			{Encoding::cp936,			{Encoding::cp936,			utils::character_encoding::Encoding::cp936,		{}}},
			{Encoding::utf16le_no_bom,	{Encoding::utf16le_no_bom,	utils::character_encoding::Encoding::utf16le,	{}}},
			{Encoding::utf16le_bom,		{Encoding::utf16le_bom,		utils::character_encoding::Encoding::utf16le,	{{0xff, 0xfe}}}},
			{Encoding::utf8_no_bom,		{Encoding::utf8_no_bom,		utils::character_encoding::Encoding::utf8,		{}}},
			{Encoding::utf8_bom,		{Encoding::utf8_bom,		utils::character_encoding::Encoding::utf8,		{{0xef, 0xbb, 0xbf}}}},
		};
		
		//添加bom
		inline void add_bom(_Out_ utils::data & data, _In_ const utils::data_view striped, _In_ const Encoding encoding)
		{
			const auto & bom = inner::table.get_existed_value(encoding).bom;
			data = bom;
			data.append(striped);
		}
		//剥离bom,返回正文
		inline void strip_bom(_Out_ utils::data_view & striped, _In_ const utils::data_view data, _In_ const Encoding encoding)
		{
			const auto & bom = inner::table.get_existed_value(encoding).bom;
			data.strip_prefix(striped, bom);
		}
		//判断编码
		//data: 文件的完整内容,包括bom
		inline void judge_encoding(_Out_ Encoding & encoding, _In_ const utils::data_view data, _In_ const utils::characters::set::Set characters_set)
		{
			//分为三步进行:
			//第一步, 看是否有bom, 如果bom对应上, 则随后的字符要符合字符编码
			//第二步, 依次检查所有没有bom的格式, 如果随后的所有字符都能在其编码中表示, 则返回
			//第三步, 没有找到, 直接抛出异常
			
			//第一步
			for (const auto & e : inner::table)
			{
				if (e.second.bom.empty())
					continue;

				if (false == data.has_prefix(e.second.bom))
					continue;

				//检查字符编码
				utils::data_view striped;
				strip_bom(striped, data, e.second.encoding);
				utils_assert(utils::character_encoding::is_valid_format(striped, e.second.character_encoding, characters_set));

				//给出编码
				encoding = e.second.encoding;
				return;
			}

			//第二步
			for (const auto & e : inner::table)
			{
				if (false == e.second.bom.empty())
					continue;

				//检查字符编码
				if (false == utils::character_encoding::is_valid_format(data, e.second.character_encoding, characters_set))
					continue;

				//给出编码
				encoding = e.second.encoding;
				return;
			}

			//第三步
			utils_throw();
		}
		//返回文本的编码
		inline utils::character_encoding::Encoding get_character_encoding(_In_ const Encoding encoding)
		{
			return inner::table.get_existed_value(encoding).character_encoding;
		}
	}

	//转换编码
	void convert(_In_ const utils::data_view from_data, _Out_ utils::data & to_data, _In_ const Encoding from_encoding, _In_ const Encoding to_encoding)
	{
		if (from_encoding == to_encoding)
			return;

		//去掉bom
		utils::data_view data;
		inner::strip_bom(data, from_data, from_encoding);

		//转换
		utils::data _data;
		utils::character_encoding::convert(data, _data, inner::get_character_encoding(from_encoding), inner::get_character_encoding(to_encoding));

		//添加bom
		inner::add_bom(to_data, _data, to_encoding);
	}

	//convert_file函数的结果汇报
	struct ConvertFile_Result
	{
		bool is_changed{};
		Encoding old_encoding{};
	};
	//把指定文件的字符编码改为指定的
	inline void convert_file(_Out_ ConvertFile_Result & result, _In_ const std::string & filename, _In_ const Encoding encoding, _In_ const utils::characters::set::Set characters_set)
	{
		//获取内容
		std::vector<uint8_t> data;
		{
			utils::FileReader input{filename};
			input.read_whole_file(data);
		}

		//判断编码
		Encoding cur_encoding{};
		inner::judge_encoding(cur_encoding, data, characters_set);

		if (cur_encoding == encoding)
		{
			result.is_changed = false;
			return;
		}
		
		//改变编码
		result.is_changed = true;
		result.old_encoding = cur_encoding;
		utils::data new_data;
		convert(data, new_data, cur_encoding, encoding);

		//更新文件内容
		{
			utils::FileWriter output{filename};
			output.write(new_data);
		}
	}
}