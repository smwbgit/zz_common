#pragma once

#include <type_traits>

namespace utils::type
{
	template <typename T>
	struct is_number : std::integral_constant<bool, std::is_integral_v<T> || std::is_floating_point_v<T>> {};

	template <typename T>
	inline constexpr bool is_number_v = is_number<T>::value;
}
