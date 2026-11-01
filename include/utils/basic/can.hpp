#pragma once

#include <stdint.h>
#include <type_traits>
#include <limits>
#include "utils/throw.hpp"
#include "utils/assert.hpp"
#include "utils/type_traits.hpp"

#include "utils/compiler/windows_header/maxmin__eliminate.inc"

namespace utils::basic::can
{
	//相乘是否会溢出
	template<typename T>
	bool mul(T a, T b)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>);

//		//无符号整数
//		if constexpr (std::is_unsigned_v<T>)
//		{
//			T t = a * b;
//			return !a || t / a == b;
//		}
//		//有符号整数
//		else
//		{
//			T t = a * b;
//			return !a || t / a == b;
//		}

		T t = a * b;
		return !a || t / a == b;
	}

	//是否可以存储在指定类型里
	template<typename T1, typename T2>
	bool store_in(_In_ const T2 from)
	{
		//简单检查
		static_assert(utils::is_integral_v<T1>, "");
		static_assert(utils::is_integral_v<T2>, "");

		if (from >= 0)
		{
			if ((uint64_t)from > (uint64_t)std::numeric_limits<T1>::max())
				return false;
		}
		if (from < 0)
		{
			if ((int64_t)from < (int64_t)std::numeric_limits<T1>::min())
				return false;
		}
		return true;
	}
	//特化
	template<typename T>
	bool store_in(_In_ const T from)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>, "");

		return true;
	}
}


#include "utils/compiler/windows_header/maxmin__restore.inc"
