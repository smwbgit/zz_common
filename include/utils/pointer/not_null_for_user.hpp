#pragma once

#include <utils/pointer/inner/Pointer.hpp>

namespace utils::pointer
{
	//对于not_null的特化：可以在初始化阶段为空，在 面向用户/使用它 时不空
	template<typename T>
	using not_null_for_user = utils::pointer::inner::Pointer<T, false, true>;
}