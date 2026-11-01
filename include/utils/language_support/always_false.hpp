#pragma once

namespace utils::language_support
{
	//不管T是什么,总为false
	//note: 为if constexpr分支里的static_assert提供支持
	//note: 为struct里的static_assert提供支持
	//example: 
	//	if constexpr(std::is_same_v<T, int>)
	//	{
	//		return 0;
	//	}
	//	else
	//	{
	//		static_assert(utils::language_support::always_false<T>);
	//	}
	template<typename>
	inline constexpr bool always_false = false;
}