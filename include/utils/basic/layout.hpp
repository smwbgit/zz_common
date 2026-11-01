#pragma once

#include <stdint.h>

//结构体的布局
namespace utils::basic::layout
{
	//获取结构体中的一个字段
	template<typename T>
	T & get(uint64_t addr, int64_t offset)
	{
		return *(T*)(addr + offset);
	}
}