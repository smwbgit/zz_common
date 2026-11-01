#pragma once

#include <utils/windows/process/open_process.hpp>
#include <utils/windows/process/terminate/wait_terminate.hpp>

namespace utils::windows::process::terminate::kill
{
	inline void by_pid(const DWORD pid)
	{
		//打开进程
		const auto hProcess = utils::windows::process::open_process(
			pid, PROCESS_TERMINATE | SYNCHRONIZE);
		
		//发出终止指令
		utils_assert(TerminateProcess(hProcess, 0));

		//等待终止
		utils::windows::process::terminate::wait_terminate(hProcess);
	}
}