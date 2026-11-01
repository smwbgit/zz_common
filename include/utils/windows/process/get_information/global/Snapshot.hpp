#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <utils/windows/handle/ManagedHandle.hpp>

namespace utils::windows::process::get_information::global
{
	namespace inner::Snapshot
	{
		struct s_iterator
		{
			bool is_end{true};
			PROCESSENTRY32W pe32{};
			utils::windows::handle::Handle hProcessSnap;

			bool operator !=(const s_iterator & o) const
			{
				//简单检查: 不应该出现这种情况
				if(!is_end && !o.is_end)
					utils_throw();

				return is_end != o.is_end;
			}
			const PROCESSENTRY32W & operator*() const
			{
				return pe32;
			}
			s_iterator& operator++()
			{
				if(!Process32NextW(hProcessSnap, &pe32))
				{
					//简单检查
					utils_assert(ERROR_NO_MORE_FILES == GetLastError());

					is_end = true;
				}
				return *this;
			}
		};
	}
	//example:
	//		std::vector<DWORD> pids;
	//		utils::windows::process::get_information::Snapshot snapshot;
	//		for(const auto & one : snapshot)
	//			pids.push_back(one.th32ProcessID);
	class Snapshot
	{
	public:
		inner::Snapshot::s_iterator begin() const
		{
			inner::Snapshot::s_iterator iterator;

			iterator.hProcessSnap.set(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
			utils_assert(iterator.hProcessSnap.value() != INVALID_HANDLE_VALUE);

			iterator.pe32.dwSize = sizeof(iterator.pe32);
			if(!Process32FirstW(iterator.hProcessSnap, &iterator.pe32))
			{
				//简单检查
				utils_assert(ERROR_NO_MORE_FILES == GetLastError());

				iterator.is_end = true;
				return iterator;
			}

			iterator.is_end = false;
			return iterator;
		}
		inner::Snapshot::s_iterator end() const
		{
			inner::Snapshot::s_iterator iterator;

			iterator.is_end = true;
			return iterator;
		}
	};
}