#pragma once

#include <stdint.h>
#include <type_traits>
#include <unordered_map>
#include <limits>
#include <functional>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

//#include "utils/compiler/windows_header/maxmin__eliminate.inc"

namespace utils::basic
{
	//保存这个整数，如果整数的值不在目标类型能保存的范围内返回false
	template<typename T1, typename T2,
		typename = std::enable_if_t<utils::is_integral_v<T1> && utils::is_integral_v<T2>>>
	bool try_store_in(_Out_ T1 & to, _In_ const T2 from)
	{
		if (from >= 0)
		{
			if ((uint64_t)from > (uint64_t)(std::numeric_limits<T1>::max)())
				return false;
		}
		if (from < 0)
		{
			if ((int64_t)from < (int64_t)(std::numeric_limits<T1>::min)())
				return false;
		}

		to = (T1)from;
		return true;
	}
	//特化
	template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
	bool try_store_in(_Out_ T & to, _In_ const T from)
	{
		to = from;
		return true;
	}

	//保存这个整数，并且确保整数的值在目标类型能保存的范围内
	template<typename T1, typename T2,
		typename = std::enable_if_t<utils::is_integral_v<T1> && utils::is_integral_v<T2>>>
	T1 store_in(_In_ const T2 from)
	{
		T1 to{};
		utils_assert(try_store_in(to, from));
		return to;
	}
	//特化
	template<typename T1, typename T2,
		typename = std::enable_if_t<utils::is_integral_v<T1> && utils::is_integral_v<T2>>>
	void store_in(_Out_ T1 & to, _In_ const T2 from)
	{
		to = store_in<T1>(from);
	}

#if 0	//todo 看看这个函数是做什么的
	template<typename T>
	void store_this_integer(T& to, uint64_t val, bool is_unsigned, uint64_t size)
	{
		static_assert(std::is_integral_v<T>);

		auto is_need_check = [&]()
		{
			if (sizeof(T) > size)
				return false;
			else if (sizeof(T) == size)
			{
				if (std::is_signed_v<T> != is_unsigned)
					return false;
			}
			return true;
		};

		if (is_need_check())
		{
			static std::unordered_map<uint64_t, std::function<int64_t(uint64_t)>> table = {
				{1, [](uint64_t val) {return (int8_t)(uint8_t)val; }},
				{2, [](uint64_t val) {return (int16_t)(uint16_t)val; }},
				{4, [](uint64_t val) {return (int32_t)(uint32_t)val; }},
				{8, [](uint64_t val) {return (int64_t)(uint64_t)val; }},
			};
			auto check = [&]()
			{
				if (is_unsigned)
				{
					if (val > (uint64_t)std::numeric_limits<T>::max())
						return false;
				}
				else
				{
					int64_t _val = table.at(size)(val);
					if (_val > 0)
					{
						if (val > (uint64_t)std::numeric_limits<T>::max())
							return false;
					}
					else
					{
						if (_val < (int64_t)std::numeric_limits<T>::min())
							return false;
					}
				}
				return true;
			};

			if (false == check())
				utils_throw();
		}

		to = static_cast<T>(val);
	}
#endif
}


//#include "utils/compiler/windows_header/maxmin__restore.inc"
