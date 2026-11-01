#pragma once

#include <utils/standard_library_support/filesystem.hpp>

//基于给定目录，生成新的完整路径
//example:  e:/b/ + e:/a/xxx.txt -> e:/b/xxx.txt
//			e:/b/ + e:/a/xxx/ -> e:/b/xxx/
namespace utils::path::text::rebase
{
	//对于单个文件
	inline auto rebase(
		const std::filesystem::path & dirname,
		const std::filesystem::path & filename)
		-> std::filesystem::path
	{
		//提取纯文件名
		const auto filename_only = filename.filename();
        
        //拼接新路径: 目录 / 文件名
		const auto new_filename = dirname / filename_only;
        
		//给出
		return new_filename;
	}
	//特化: 对于一些文件
	inline auto rebase(
		const std::filesystem::path & dirname,
		const std::vector<std::filesystem::path> & filenames)
		-> std::vector<std::filesystem::path>
	{
		//准备数据
		std::vector<std::filesystem::path> new_filenames;

		//对于每一个文件名
		for(const auto & filename : filenames)
		{
			//转发
			auto new_filename = rebase(dirname, filename);

			//保存
			new_filenames.push_back(std::move(new_filename));
		}

		//给出
		return new_filenames;
	}
}