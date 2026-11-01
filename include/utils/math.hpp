#pragma once

#include <stdint.h>

namespace utils::math
{
	//传入元素总数和一块的大小，返回块数
	//e.g.	{0,  5} -> 0
	//		{1,  5} -> 1
	//		{4,  5} -> 1
	//		{5,  5} -> 1
	//		{6,  5} -> 2
	//		{9,  5} -> 2
	//		{10, 5} -> 2
	//		{11, 5} -> 3
	inline uint64_t get_block_n(_In_ const uint64_t element_n, _In_ const uint64_t element_n_per_block)
	{
		return (element_n + element_n_per_block - 1) / element_n_per_block;
	}

	
}