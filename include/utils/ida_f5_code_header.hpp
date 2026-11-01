#pragma once

#include <stdint.h>

//从ida复制过来的代码里的用到的一些类型和函数
namespace utils::ida::f5_code_header
{
	class my__int128
	{
	private:
		uint64_t data[2];
	};
//#pragma warning(push)
//#pragma warning(disable: 4235)	//error C4235: 使用了非标准扩展: 不支持在此结构上使用“__int128”关键字
//	using __int128 = my__int128;
//#pragma warning(pop)
#define __int128 my__int128

	using _BYTE = uint8_t;
	using _WORD = uint16_t;
	using _DWORD = uint32_t;
	using _QWORD = uint64_t;
	using _OWORD = __int128;
//#define _BYTE uint8_t
//#define _DWORD uint32_t
//#define _QWORD uint64_t
//#define _OWORD __int128

	inline void memset64(void *dst, uint64_t value, uint64_t count)
	{
		for (uint64_t i = 0; i < count; i++)
			((uint64_t*)dst)[i] = value;
	}
#ifndef LODWORD
	template<typename T>
	_DWORD & LODWORD(T & value)
	{
		static_assert(sizeof(T) == 8);
		return *(_DWORD*)&value;
	}
#endif
#ifndef LOBYTE
	template<typename T>
	_BYTE & LOBYTE(T & value)
	{
		static_assert(sizeof(T) == 4);
		return *(_BYTE*)&value;
	}
#endif
}