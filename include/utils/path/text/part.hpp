#pragma once

#include <utils/standard_library_support/filesystem.hpp>
#include <string>
#include <vector>

//对路径的某一部分(比如，扩展名/文件名/上级目录)，进行读写
namespace utils::path::text::part
{
	//nothing
}

//查询
namespace utils::path::text::part::get
{
	//获取文件名
	//example:  e:/a/xxx.txt ->  xxx.txt
	//			e:/a/xxx/ -> xxx
	inline std::u8string filename_part(const std::filesystem::path & full_name)
	{
		//形如 e:/a/xxx.txt
		if(full_name.has_filename())
		{
			return full_name.filename().u8string();
		}
		//形如 e:/a/xxx/
		else
		{
			//简单检查
			utils_assert(full_name.has_parent_path());

			return full_name.parent_path().filename().u8string();
		}
	}
}
//替换
namespace utils::path::text::part::set
{
	//设置文件名
	//example:  e:/a/xxx.txt ->  e:/a/[new]
	//			e:/a/xxx/ -> e:/a/[new]/
	inline std::filesystem::path filename_part(const std::filesystem::path & full_name, const std::u8string & text)
	{
		//准备数据
		auto _full_name = full_name;

		//形如 e:/a/xxx.txt
		if(full_name.has_filename())
		{
			//设置文件名
			_full_name.replace_filename({text});
		}
		//形如 e:/a/xxx/
		else
		{
			//准备数据
			_full_name = full_name.parent_path();

			//简单检查: 还有上级目录名
			utils_assert(_full_name != full_name);

			//设置文件名
			_full_name.replace_filename({text});

			//再把后边的'/'补回来
			_full_name /= "";
		}

		//给出
		return _full_name;
	}
}
//追加
namespace utils::path::text::part::append
{
	//向 文件名/目录名 后，同级追加后缀
	//example:  e:/a/xxx.txt -> e:/a/xxx.txt[new add]
	//			e:/a/xxx/ -> e:/a/xxx[new add]/
	inline std::filesystem::path at_same_level(const std::filesystem::path & path, const std::u8string & text)
	{
		//获取 文件名部分
		auto filename = utils::path::text::part::get::filename_part(path);

		//追加后缀
		filename += text;

		//设置 文件名部分
		const auto new_path = utils::path::text::part::set::filename_part(path, filename);

		//给出
		return new_path;
	}
}