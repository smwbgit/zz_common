#pragma once

#include <stdint.h>

namespace utils::basic
{
	//比较一个i64和一个u64, 小于返回-1, 大于返回1, 等于返回0
	inline int64_t cmp_i64_u64(int64_t v0, uint64_t v1)
	{
		if (v0 < 0) return -1;
		if ((uint64_t)v0 < v1) return -1;
		if ((uint64_t)v0 == v1) return 0;
		return 1;
	}
}