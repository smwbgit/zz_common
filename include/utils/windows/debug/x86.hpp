#pragma once

#include <windows.h>
#include "utils/throw.hpp"

namespace utils::windows::cpu::x86
{
	//指定的硬件断点是否命中
	//breakpoint_index: 硬件断点的索引，[0, 3]
	inline bool is_hardware_breakpoint_hit(_In_ const CONTEXT & context, _In_ uint64_t breakpoint_index)
	{
		switch(breakpoint_index)
		{
		case 0:	return context.Dr6 & 0x1;
		case 1:	return context.Dr6 & 0x2;
		case 2:	return context.Dr6 & 0x4;
		case 3:	return context.Dr6 & 0x8;
		}

		utils_throw();
	}
	//启用一个硬件执行断点
	//breakpoint_index: 硬件断点的索引，[0, 3]
	//instruction_addr: 指令地址
	inline void enable_hardware_execute_breakpoint(_Inout_ CONTEXT & context, _In_ uint64_t breakpoint_index, _In_ uint32_t instruction_addr)
	{
		switch(breakpoint_index)
		{
		case 0:
			context.Dr0 = instruction_addr;
			context.Dr7 |= 0x1;
			return;
		case 1:
			context.Dr1 = instruction_addr;
			context.Dr7 |= 0x4;
			return;
		case 2:
			context.Dr2 = instruction_addr;
			context.Dr7 |= 0x10;
			return;
		case 3:
			context.Dr3 = instruction_addr;
			context.Dr7 |= 0x40;
			return;
		}

		utils_throw();
	}
	//取消一个硬件断点
	//breakpoint_index: 硬件断点的索引，[0, 3]
	inline void disable_hardware_breakpoint(_Inout_ CONTEXT & context, _In_ uint64_t breakpoint_index)
	{
		switch(breakpoint_index)
		{
		case 0:
			context.Dr7 &= ~0x1;
			return;
		case 1:
			context.Dr7 &= ~0x4;
			return;
		case 2:
			context.Dr7 &= ~0x10;
			return;
		case 3:
			context.Dr7 &= ~0x40;
			return;
		}

		utils_throw();
	}
}
