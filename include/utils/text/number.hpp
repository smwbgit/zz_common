#pragma once

#include <utils/text/value.hpp>
#include <utils/throw.hpp>

//数字相关
namespace utils::text::number
{
	//是否都为数字
	inline bool is_all_number(const std::u8string & text)
	{
		for(const auto c : text)
		{
			if(c >= '0' && c <= '9')
				continue;

			return false;
		}
		return true;
	}

#pragma region 文本和数字之间转换
	//从文本转为数字
	inline void from_text(int64_t & value, const std::u8string & text, uint64_t base = 10)
	{
		//简单检查
		utils_assert(is_all_number(text));

		size_t pos{};
		utils_std_exception_not_throwed(value = std::stoll((const std::string &)text, &pos, (int)base));

		//简单检查
		utils_assert(pos == text.length());
	}
	//从数字转为文本
	inline std::u8string to_text(const int64_t value)
	{
		const auto _text = std::to_string(value);

		return (const std::u8string &)_text;
	}
	//特化
	inline void to_text(std::u8string & text, const int64_t value)
	{
		//转发
		text = to_text(value);
	}
#pragma endregion
}