#pragma once

#include <utils/windows/unwind.hpp>
#include <utils/windows/file/FileMapping.hpp>
#include <utils/real_path.hpp>

namespace utils::windows::unwind::small_program
{
	//检查windows/system32下的所有dll的unwind_info
	inline void check_unwind_info_in_windows_system32()
	{
		std::vector<std::u8string> filenames = utils::real_path::get_filenames_in_directory(
			u8R"(C:\Windows\System32)", false,
			[](const std::u8string & filename)
			{
				return filename.ends_with(u8".dll");
			});
	
		std::vector<std::u8string> filenames_not_x64;
		std::vector<std::u8string> filenames_checked;

		for (const auto & filename : filenames)
		{
			if(filename == u8R"(C:\Windows\System32\sqlncli11.dll)") continue;	//跳过未知问题：start+1

			utils::windows::file::FileMapping file_mapping{filename};
			utils::windows::pe::PeViewer pe_viewer{file_mapping.addr(), true};
		
			if(!pe_viewer.is_x64())
			{
				filenames_not_x64.push_back(filename);
				continue;
			}

			utils::windows::unwind::check_unwind_info(pe_viewer);
			filenames_checked.push_back(filename);
		}

		Sleep(1);
	}
}