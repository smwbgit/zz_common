#pragma once

#include <string>
#include <string_view>
#include <utils/assert.hpp>

//提取
namespace utils::text::extract
{
	//获取指定前后缀中间的字符串
	//text: 文本
	//pos: 开始搜索位置
	//prefix: 前缀
	//postfix: 后缀
	//result: 中间的字符串
	//return: 如果没找到，则返回text.npos，否则返回下一个搜索位置
	inline uint64_t get_word_by_prefix_and_postfix(std::string_view text, std::string_view prefix, std::string_view postfix, uint64_t pos, std::string & result)
	{
		pos = text.find(prefix, pos);

		if (pos == text.npos)
			return pos;

		//查找结尾
		auto pos2 = text.find(postfix, pos + prefix.length());
		utils_assert(pos2 != text.npos);

		result = text.substr(pos, pos2-pos).substr(prefix.length());
		return pos2 + postfix.length();
	}
	//特化
	inline std::string get_word_by_prefix_and_postfix(std::string_view text, std::string_view prefix, std::string_view postfix)
	{
		std::string result;
		auto pos = get_word_by_prefix_and_postfix(text, prefix, postfix, 0, result);

		//简单检查
		utils_assert(pos != text.npos);

		return result;
	}
}