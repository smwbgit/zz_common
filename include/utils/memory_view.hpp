#pragma once

#include <stdint.h>

namespace utils
{
	//表示一个内存区域
	class memory_view
	{
	public:
		const uint8_t* addr{};
		uint64_t size{};

	public:
		//获得一个子区域
		memory_view sub(uint64_t offset, uint64_t size) const
		{
			return {addr + offset, size};
		}
	};
}