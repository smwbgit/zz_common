#pragma once

#include <string>
#include <stdarg.h>
#include <utils/assert.hpp>

//格式化
namespace utils::text::format
{
	//格式化
	/*
		va_list args;
		va_start(args, format);
		va_end(args);
	*/
	std::u8string format_vargs(const char8_t* format, va_list va)
	{
		int _length{};
		{
			va_list args1;
			va_copy(args1, va);
			_length = std::vsnprintf(nullptr, 0, (const char*)format, args1);
			va_end(args1);

			//简单检查
			utils_assert(_length >= 0);
		}

		va_list args2;
		va_copy(args2, va);	//一个va_list只能用一遍，再用要用va_start()或va_copy()重新创建
		std::u8string text;
		text.resize(_length);
		int re = std::vsnprintf((char*)text.data(), text.length() + 1, (const char*)format, args2);
		va_end(args2);
		utils_assert(re == text.length());	//简单检查
		return text;
	}
	//特化
	inline std::u8string format(const char8_t* format, ...)
	{
		va_list va;
		va_start(va, format);
		auto text = format_vargs(format, va);
		va_end(va);
		return text;
	}
	//特化
	inline std::string format_vargs(const char* format, va_list va)
	{
		auto text_u8 = format_vargs((const char8_t*)format, va);
		return {(const char*)text_u8.c_str()};
	}
	//特化
	inline std::string format(const char* format, ...)
	{
		va_list va;
		va_start(va, format);
		auto text = format_vargs(format, va);
		va_end(va);
		return text;
	}
}