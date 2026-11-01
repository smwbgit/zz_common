#pragma once

#include <debugapi.h>
#include "utils/real_path.hpp"

namespace utils::windows::vs
{
	//向输出窗口添加一条消息
	inline void log(const std::string & message)
	{
		::OutputDebugStringA(message.c_str());
	}
	//插入一个断点
	inline void debug_break()
	{
		if (::IsDebuggerPresent())
			::DebugBreak();
	}

#pragma region vs的安装位置
	namespace inner
	{
		inline const std::string vs_dir = R"(e:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE)";
		inline const std::string filename_devenv = vs_dir + R"(\devenv.exe)";
	}
	//是否安装了vs
	inline bool is_exist_visual_studio()
	{
		//todo 自动找出vs目录    https://github.com/Microsoft/vswhere

		return utils::real_path::is_file_exist(inner::filename_devenv);
	}
	//获得devenv.exe的完整路径
	inline void get_devenv_filename(_Out_ std::string& filename)
	{
		utils_assert(is_exist_visual_studio());

		filename = inner::filename_devenv;
	}
#pragma endregion
}