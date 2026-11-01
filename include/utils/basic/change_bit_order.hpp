#pragma once

#include <type_traits>
#include <vector>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

namespace utils::basic
{
	//对普通类型的转换
	template<typename T, typename = std::enable_if_t<utils::is_arithmetic_v<T>>>
	void change_bit_order(T & value)
	{
		//简单检查
		static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

		T new_value{};
		const uint64_t bit_n = sizeof(T)*8;
		for (uint64_t i = 0; i < bit_n; i++)
		{
			if (value & (1 << i))
				new_value |= (1 << (bit_n - i - 1));
		}
		value = new_value;
	}
}
