#pragma once

#include <utils/windows/process/memory.hpp>
#include <utils/windows/process.hpp>
#include <utils/FileWriter.hpp>
#include <utils/FileReader.hpp>

namespace utils::windows::process::memory::小工具
{
	inline void 转储所有内存(const std::filesystem::path & filename, DWORD pid = GetCurrentProcessId())
	{
		utils::windows::process::temporary_freeze_process([&]()
		{
			utils::FileWriter output{filename};
			auto blocks = utils::windows::process::memory::get_memory_blocks(pid);
			for (const auto & one : blocks)
			{
				if (utils::windows::process::memory::is_commit(one.info) &&
					!utils::windows::process::memory::is_guard(one.info))
				{
					utils::windows::process::memory::temporary_change_protect(one.info, [&]()
					{
						//保存
						output.write(one.info);
						if (pid == GetCurrentProcessId())
						{
							output.write(one.info.BaseAddress, one.info.RegionSize);
						}
						else
						{
							auto data = utils::windows::process::memory::read_memory(
								pid, (uint64_t)one.info.BaseAddress, one.info.RegionSize);
							output.write(data);
						}
					});
				}
			}
		}, pid);
		
	}

	struct s_memory_block
	{
		MEMORY_BASIC_INFORMATION info{};
		utils::data data;
	};
	//读取 "转储所有内存()" 函数所生成的文件
	inline std::vector<s_memory_block> 读取转储结果(const std::filesystem::path & filename)
	{
		utils::FileReader input{filename};
		std::vector<s_memory_block> blocks;
		while (!input.is_readed_all())
		{
			s_memory_block b;
			input.read(b.info);
			input.read(b.data, b.info.RegionSize);
			blocks.push_back(std::move(b));
		}
		return blocks;
	}
}
