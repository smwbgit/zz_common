#pragma once

namespace utils::text::encoding::language::chinese
{
	//是否为中文字符
	inline bool is_chinese_char(char16_t c)
	{
		return c >= 0x4E00 && c <= 0x9FFF;	//仅常见的中文, 不包括偏僻地名和人名用字，也不包括甲骨文之类的
	}
}