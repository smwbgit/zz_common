#pragma once

#include <stdint.h>

namespace utils::integer_type
{
	using u1 = uint8_t;
	using u2 = uint16_t;
	using u4 = uint32_t;
	using u8 = uint64_t;

	using s1 = int8_t;
	using s2 = int16_t;
	using s4 = int32_t;
	using s8 = int64_t;
}

namespace utils
{
	//导出
	using namespace utils::integer_type;
}