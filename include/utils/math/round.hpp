#pragma once

#include <math.h>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

namespace utils::math::round
{
	//取整, 以变大的方式
	template<typename T>
	inline T bigger(T value, uint64_t page_size)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>);
		utils_assert(value >= 0);

		if(value%page_size == 0)
			return value;
		else
		{
			auto re = value + (page_size - value % page_size);

			//简单检查
			utils_assert(re > value);

			return re;
		}
	}
}