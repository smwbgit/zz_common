#pragma once

#include <concepts>

namespace utils::concepts
{
	//支持随机读取
	template<typename T>
	concept RandomReader = requires(T input)
	{
		{ input.read_random(
			(uint64_t) /*position*/ 0,
			(uint8_t*) /*buffer*/ nullptr,
			(uint64_t) /*size*/ 0)
		} -> std::same_as<void>;
	};
}