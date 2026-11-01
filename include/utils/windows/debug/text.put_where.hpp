#pragma once

#include <windows.h>
#include <string>
#include <stdint.h>
#include <utils/assert.hpp>
#include <utils/text/encoding.hpp>

namespace utils::windows::text
{
	//addr: UNICODE_STRING结构的首地址
	inline std::wstring from_UNICODE_STRING(uint64_t addr)
	{
		auto size = *(uint16_t*)addr;
		auto max_size = *(uint16_t*)(addr + 0x2);
		auto text = *(uint64_t*)(addr + 0x8);

		//简单检查
		utils_assert(size <= max_size + 2);	//因为最后要有两个0
		utils_assert(*(uint16_t*)(text + size) == 0);

		return (wchar_t*)text;
	};
	//特化
	inline void from_UNICODE_STRING(std::string & text, uint64_t addr)
	{
		auto _text = from_UNICODE_STRING(addr);
		text = (std::string &&)utils::text::encoding::unicode::utf16_to_utf8((const std::u16string &)_text);
	}
}