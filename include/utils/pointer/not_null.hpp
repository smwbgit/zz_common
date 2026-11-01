#pragma once

#include <utils/pointer/inner/Pointer.hpp>

namespace utils::pointer
{
	//表示一个非空指针
	template<typename T>
	using not_null = inner::Pointer<T, false>;
}