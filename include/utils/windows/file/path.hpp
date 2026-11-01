#pragma once

#include <string>
#include "utils/uuid.hpp"
#include "utils/assert.hpp"

namespace utils::windows::path
{
	//获取windows系统临时目录
	void get_system_temp_directory(_Out_ std::string & dirname)
	{
		char tmp[MAX_PATH+1]{};
		DWORD re = GetTempPathA(sizeof(tmp), tmp);
		utils_assert(re > 0);
		dirname = tmp;

		utils_assert(dirname[dirname.size()-1] == '\\');
		dirname.resize(dirname.size() - 1);
	}
	//获取一个新的临时文件名。文件位于windows系统的临时目录里。返回绝对路径
	void get_new_filename_in_system_temp_directory(_Out_ std::string & filename_temp)
	{
		std::string dirname;
		utils::windows::path::get_system_temp_directory(dirname);
		std::string uuid_text;
		utils::uuid_function::get_string_with_dash(uuid_text);
		filename_temp = dirname + "\\" + uuid_text + ".tmp";
	}

}
