#pragma once

#include <windows.h>
#include <utils/windows/handle/ManagedHandle.hpp>

namespace utils::windows::process
{
	inline auto try_open_process(DWORD pid, DWORD dwDesiredAccess, BOOL bInheritHandle = FALSE)
		-> std::optional<utils::windows::handle::Handle>
	{
		const auto hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, pid);
		if(!hProcess)
		{
			//简单检查
			utils_assert(ERROR_ACCESS_DENIED == GetLastError());

			return {};
		}
		return {{hProcess}};
	}
	//特化
	inline auto open_process(DWORD pid, DWORD dwDesiredAccess, BOOL bInheritHandle = FALSE)
		-> utils::windows::handle::Handle
	{
		//转发
		auto hProcess = try_open_process(pid, dwDesiredAccess, bInheritHandle);

		//简单检查
		utils_assert(hProcess);

		//给出
		return std::move(hProcess.value());
	}
}