#pragma once

#include <stdint.h>

namespace utils::basic::sign_extend
{
	inline uint64_t u8_to_u64(uint8_t value)
	{
		return (uint64_t)(int64_t)(int8_t)value;
	}
	inline uint64_t u16_to_u64(uint16_t value)
	{
		return (uint64_t)(int64_t)(int16_t)value;
	}
	inline uint64_t u32_to_u64(uint32_t value)
	{
		return (uint64_t)(int64_t)(int32_t)value;
	}
}