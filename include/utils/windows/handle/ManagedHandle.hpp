#pragma once

#include <optional>
#include <windows.h>
#include <utils/ManagedHandle.hpp>

namespace utils::windows::handle
{
	template<typename T>
	class ManagedHandle;
	//特化
	template<>
	class ManagedHandle<HANDLE> : public utils::ManagedHandle<HANDLE>
	{
	public:
		ManagedHandle(std::optional<HANDLE> handle = {})
			: utils::ManagedHandle<HANDLE>::ManagedHandle{handle}
		{
			m_close_handler = [](HANDLE handle)
			{
				if(handle)
					utils_assert(CloseHandle(handle));
			};
		}
	};
	//特化
	template<>
	class ManagedHandle<HMODULE> : public utils::ManagedHandle<HMODULE>
	{
	public:
		ManagedHandle(std::optional<HMODULE> handle = {})
			: utils::ManagedHandle<HMODULE>::ManagedHandle{handle}
		{
			m_close_handler = [](HMODULE handle)
			{
				utils_assert(FreeLibrary(handle));
			};
		}
	};

	using Handle = ManagedHandle<HANDLE>;
}