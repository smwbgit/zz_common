#pragma once

#include <stdint.h>

namespace utils::basic
{
	//检查传入的对象是否全是0
	template<typename T>
	inline bool is_zero(const T & value)
	{
		for(uint64_t i = 0; i < sizeof(T); i++)
		{
			//检查一个字节
			if(*((uint8_t*)&value + i))
				return false;
		}
		return true;
	}

	//整除
	uint64_t exactly_divide(uint64_t 除数, uint64_t 被除数)
	{
		//简单检查
		utils_assert(除数 % 被除数 == 0);

		return 除数 / 被除数;
	}
}