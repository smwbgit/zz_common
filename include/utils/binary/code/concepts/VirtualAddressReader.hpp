#pragma once

#include <utils/concepts/RandomReader.hpp>

namespace utils::binary::code::concepts
{
	//可读取内存空间（虚拟地址）里的数据
	template<typename T>
	concept VirtualAddressReader = utils::concepts::RandomReader<T>;
}