#pragma once

#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

namespace utils::concepts
{
	//整数
	template<typename T>
	concept integer = utils::is_integral_v<T>;
}