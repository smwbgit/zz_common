#pragma once

#include <windows.h>

namespace utils::windows::process::terminate
{
	//等待进程结束
	inline void wait_terminate(HANDLE hProcess)
	{
		utils_assert(WAIT_OBJECT_0 == WaitForSingleObject(hProcess, INFINITE));
	}
}