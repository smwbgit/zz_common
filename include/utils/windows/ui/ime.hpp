#pragma once

#include <imm.h>
#pragma comment(lib, "Imm32.lib")
#include "utils/assert.hpp"

namespace utils::windows::ime
{
	//改变进程的输入法为英文输入法
	inline void change_to_english_language()
	{
		//载入
		HKL handle = LoadKeyboardLayoutA("00000409", 0);
		utils_assert(handle);
		
		//使用
		auto re = ActivateKeyboardLayout(handle, KLF_SETFORPROCESS);
		utils_assert(re);
	}
}