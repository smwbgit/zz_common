#pragma once

#include <windows.h>
#include <utils/text/encoding.hpp>

//控制台
namespace utils::windows::ui::console
{
	//显示文本
	inline void write_text(const std::u8string & text)
	{
		//获取输出句柄
		const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

		//简单检查
		utils_assert(handle != INVALID_HANDLE_VALUE && handle != NULL);

		//准备数据
		const auto text_u16 = utils::text::encoding::utf8_to_utf16(text);

		//写入文本
		DWORD writed_length{};
		utils_assert(WriteConsoleW(handle, text_u16.c_str(), (DWORD)text_u16.length(), &writed_length, NULL));

		//简单检查
		utils_assert(writed_length == text_u16.length());
	}
}