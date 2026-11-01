#pragma once

#include <windows.h>
#include "utils/assert.hpp"

namespace utils::windows::api::pipe
{
	//检查指定的管道是否有数据可以读取
	inline bool is_data_reached(HANDLE pipe)
	{
		DWORD bytes_avail{};
		utils_assert(PeekNamedPipe(pipe, NULL, 0, NULL, &bytes_avail, NULL));
		return bytes_avail > 0;
	}
}