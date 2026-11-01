#pragma once

#include <immintrin.h>
#include <stdint.h>

namespace utils::cpu
{
	inline void get_gs(uint64_t & gs)
	{
		gs = _readgsbase_u64();
	}
}