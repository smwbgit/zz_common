#pragma once

#include <utils/windows/process.hpp>

//暂停
namespace utils::windows::process::suspend
{
	//尝试临时暂停一个线程
	//meet_suspend_success: 暂停成功后, 重新恢复前, 调用
	//return: 是否成功
	inline bool try_temporary_suspend_thread(DWORD tid, std::function<void()> meet_suspend_success)
	{
		if(tid == GetCurrentThreadId()) return false;

		auto thread = utils::windows::process::open_thread(tid, THREAD_SUSPEND_RESUME);
		auto state = SuspendThread(thread);
		utils_assert((DWORD)-1 != state || GetLastError() == ERROR_ACCESS_DENIED);
		bool is_success = (DWORD)-1 != state;
		if(is_success)
		{
			meet_suspend_success();

			utils_assert((DWORD)-1 != ResumeThread(thread));
		}
		return is_success;
	}
}