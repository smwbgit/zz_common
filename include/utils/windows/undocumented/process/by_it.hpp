#pragma once

#include <utils/windows/process/open_process.hpp>
#include <utils/windows/undocumented/process.hpp>

namespace utils::windows::undocumented::process::by_it
{
	inline DWORD get_ppid(DWORD pid)
	{
		using PROCESS_BASIC_INFORMATION = utils::windows::undocumented::process::PROCESS_BASIC_INFORMATION;

		//delete_this_line::PROCESS_QUERY_INFORMATION
		const auto hProcess = utils::windows::process::open_process(
			pid, PROCESS_QUERY_LIMITED_INFORMATION);

		PROCESS_BASIC_INFORMATION pbi{};
		NTSTATUS status = utils::windows::undocumented::process::NtQueryInformationProcess(
			hProcess,
			0, // ProcessBasicInformation
			&pbi,
			sizeof(pbi),
			NULL);
		utils_assert(NT_SUCCESS(status));

		return (DWORD)pbi.InheritedFromUniqueProcessId;
	}
}