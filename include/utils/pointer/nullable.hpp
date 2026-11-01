#pragma once

#include <utils/pointer/inner/Pointer.hpp>

namespace utils::pointer
{
	//表示一个可空指针
	template<typename T>
	using nullable = utils::pointer::inner::Pointer<T, true>;
}