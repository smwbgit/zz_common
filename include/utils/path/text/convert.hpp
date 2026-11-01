#pragma once

#include <utils/standard_library_support/filesystem.hpp>
#include <string>
#include <vector>

//在 std::u8string 和 std::filesystem::path 间，做转换
namespace utils::path::text::convert
{
	inline std::vector<std::u8string> to_u8(const std::vector<std::filesystem::path> & paths)
	{
		//准备数据
		std::vector<std::u8string> paths_u8;

		//对于每个路径
		for(const auto & path : paths)
		{
			//转换并保存
			paths_u8.push_back(path.u8string());
		}

		//给出
		return paths_u8;
	}
}