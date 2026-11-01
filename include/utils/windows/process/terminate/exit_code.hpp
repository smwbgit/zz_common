#pragma once

#include <utils/windows/process/terminate/wait_terminate.hpp>

namespace utils::windows::process::terminate::exit_code
{
	//获取退出码
	inline DWORD get_exit_code_of_terminated_process(HANDLE hProcess)
	{
		DWORD exit_code{};
		utils_assert(GetExitCodeProcess(hProcess, &exit_code));
		return exit_code;
	}
	//特化: 等待进程结束
	inline DWORD wait_exit_code(HANDLE hProcess)
	{
		//等待进程结束
		utils::windows::process::terminate::wait_terminate(hProcess);

		//获取退出码
		const auto exit_code = get_exit_code_of_terminated_process(hProcess);

		//给出
		return exit_code;
	}
}