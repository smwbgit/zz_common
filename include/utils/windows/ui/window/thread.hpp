#pragma once

#include <windows.h>
#include <utils/callback/CallbackAdapter.hpp>
#include <utils/assert.hpp>

namespace utils::windows::ui::window::thread
{
	//统计当前线程拥有的窗口数量
	//note: 不包括子窗口
	inline uint64_t count_thread_window()
	{
		uint64_t count{};
		utils::callback::CallbackAdapter< 1, BOOL, HWND, LPARAM > callback {
			[&count](HWND hwnd, LPARAM lParam) ->BOOL  {
				count++;
				return TRUE; /* 继续枚举 */  }};
		auto re = EnumThreadWindows(GetCurrentThreadId(), callback.callback_stdcall(), callback.param());

		//简单检查
		utils_assert(re || ERROR_SUCCESS == GetLastError());
    
		return count;
	}
}