#pragma once

#include <utils/integer_type.hpp>
#include <utils/type_traits.hpp>

namespace utils::basic
{
	//按指定字节对齐
	template<typename T, typename = utils::enable_for_same<T, u8>>
	inline T align(T value, u8 size)
	{
		// 简单检查
		utils_assert(size > 0);

		// 对齐
		const T value_2 = (value + size - 1) / size * size;

		// 给出
		return value_2;
	}
}