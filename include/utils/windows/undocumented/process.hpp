#pragma once

#include <windows.h>
#include <winternl.h>
#include <utils/windows/module/get_function_address.hpp>

namespace utils::windows::undocumented::process
{
	//进程信息结构
	struct PROCESS_BASIC_INFORMATION
	{
		NTSTATUS ExitStatus;
		PPEB PebBaseAddress;
		ULONG_PTR AffinityMask;
		KPRIORITY BasePriority;
		ULONG_PTR UniqueProcessId;
		ULONG_PTR InheritedFromUniqueProcessId;
	};

	inline NTSTATUS NtQueryInformationProcess(
		HANDLE ProcessHandle,
		ULONG ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength)
	{
		//函数指针类型
		using function_t = NTSTATUS (NTAPI*)(
			HANDLE ProcessHandle,
			ULONG ProcessInformationClass,
			PVOID ProcessInformation,
			ULONG ProcessInformationLength,
			PULONG ReturnLength);
	 
		static const function_t function =
			utils::windows::module::get_function_address<function_t>(
				L"ntdll.dll", "NtQueryInformationProcess");

		return function(
			ProcessHandle,
			ProcessInformationClass,
			ProcessInformation,
			ProcessInformationLength,
			ReturnLength);
	}
}