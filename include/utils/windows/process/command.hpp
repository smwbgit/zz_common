#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows::command
{
	//执行一个命令，并等待执行完毕
	//is_hide: 是否隐藏窗口
	//return: 命令的退出码
	inline uint32_t run(const std::string & command, bool is_hide = true)
	{
		STARTUPINFOA si = {sizeof(si)};
		GetStartupInfoA(&si);
		si.dwFlags=STARTF_USESHOWWINDOW;
		si.wShowWindow=is_hide?SW_HIDE:SW_SHOW;
//		si.wShowWindow=SW_SHOW;

		PROCESS_INFORMATION pi{};
		auto _command = command;
		utils_assert(CreateProcessA(NULL,(char*)_command.c_str(),NULL,NULL,FALSE,NULL,NULL,NULL,&si,&pi));

		utils_assert(WAIT_OBJECT_0 == WaitForSingleObject(pi.hProcess,INFINITE));

		DWORD exit_code{};
		utils_assert(GetExitCodeProcess(pi.hProcess, &exit_code));
		utils_assert(CloseHandle(pi.hProcess));
		utils_assert(CloseHandle(pi.hThread));
		return exit_code;
	}
}