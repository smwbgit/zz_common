#pragma once

#include <utils/windows/handle/ManagedHandle.hpp>
#include <utils/windows/process/start_process/cmd.hpp>
#include <utils/windows/process/terminate/exit_code.hpp>

//模拟用户双击操作
namespace utils::windows::process::start_process::open_by_double_click
{
	struct not_wait_termination__params
	{
		std::u8string full_filename;
		std::vector<std::u8string> params;
	};
	inline utils::windows::handle::Handle not_wait_termination(const not_wait_termination__params & params)
	{
		//拼出完整命令
		std::wstring cmd;
		utils::windows::process::start_process::cmd::get_whole_cmd(
			cmd, params.full_filename, params.params);

		//准备数据
		STARTUPINFOW si{};
		si.cb = sizeof(si);

		//创建
		PROCESS_INFORMATION pi{};
		utils_assert(CreateProcessW(
			NULL,
			(wchar_t*)cmd.c_str(),
			NULL,
			NULL,
			FALSE,
			CREATE_DEFAULT_ERROR_MODE | DETACHED_PROCESS,
			NULL,
			NULL,
			&si,
			&pi));
		utils_assert(CloseHandle(pi.hThread));

		//给出
		utils::windows::handle::Handle hProcess;
		hProcess.set(pi.hProcess);
		return hProcess;
	}

	struct wait_termination__params
	{
		std::u8string full_filename;
		std::vector<std::u8string> params;
	};
	//return: 进程的退出码
	inline DWORD wait_termination(const wait_termination__params & params)
	{
		not_wait_termination__params _params;
		_params.full_filename = params.full_filename;
		_params.params = params.params;
		const auto hProcess = not_wait_termination(_params);
		
		const auto exit_code = utils::windows::process::terminate::exit_code::
			wait_exit_code(hProcess);
		return exit_code;
	}
}