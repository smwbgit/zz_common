#pragma once

#include <utils/windows/process/get_information/global/Snapshot.hpp>

namespace utils::windows::process::get_information::global
{
	//return: 返回的是pid
	inline std::vector<DWORD> get_all_process()
	{
		using Snapshot = utils::windows::process::get_information::global::Snapshot;

		std::vector<DWORD> pids;
		Snapshot snapshot;
		for(const auto & one : snapshot)
			pids.push_back(one.th32ProcessID);
		return pids;
	}
}