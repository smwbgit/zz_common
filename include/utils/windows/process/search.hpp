#pragma once

#include <utils/windows/process/get_information/global.hpp>
#include <utils/windows/process/get_information/single_one.hpp>

namespace utils::windows::process::search
{
	inline std::optional<DWORD> by_full_filename(const std::u8string & full_filename)
	{
		//获得所有进程
		const auto pids = utils::windows::process::get_information::global::get_all_process();

		//检查每个进程
		std::vector<DWORD> results;
		for(const auto pid : pids)
		{
			//查询文件名
			const auto filename = utils::windows::process::get_information::single_one::
				get_full_filename(pid);

			//比较并保存
			if(filename.has_value() && filename.value() == full_filename)
				results.push_back(pid);
		}

		//简单检查
		utils_assert(results.size() <= 1);

		//给出
		if(results.empty())
			return {};
		else
			return {results[0]};
	}
}