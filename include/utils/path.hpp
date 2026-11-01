#pragma once

#include <utils/standard_library_support/filesystem.hpp>
#include <utils/u8string.hpp>
#include <utils/assert.hpp>

namespace utils::path
{
	//检查扩展名是否为指定值
	//extension: 扩展名    e.g.   ".txt"
	//is_case_sensitive: 是否大小写敏感
	inline void extension_is_this(const std::filesystem::path & filename, const std::u8string & extension, bool is_case_sensitive)
	{
		if (is_case_sensitive)
			utils_assert(filename.extension().u8string() == extension);
		else
			utils_assert(utils::u8string(filename.extension().u8string()).view().compare_is_same(extension, true));
	}

	//获取上级目录
	//e.g.:
	//	The parent path of "/var/tmp/example.txt" is "/var/tmp"
	//	The parent path of "/" is "/"
	//	The parent path of "/var/tmp/." is "/var/tmp"
	inline void get_parent_path(std::string & parent_path, const std::string & filename)
	{
		namespace fs = std::filesystem;
		fs::path p{filename};
		parent_path = p.parent_path().string();
	}

	//获取扩展名前面的部分     xxx.abc ->  xxx
	inline void get_stem(std::string & stem, const std::string & filename)
	{
		namespace fs = std::filesystem;
		fs::path p{filename};
		stem = p.stem().string();
	}

#if 0	//请改用 utils::path::text::part::get::filename_part
	//获取文件名     e:/a/xxx.txt ->  xxx.txt
	inline std::u8string get_filename(_In_ std::u8string_view full_name)
	{
		return std::filesystem::path{full_name}.filename().u8string();
	}
#endif

	//增加扩展名前面的部分     c:/xxx.abc ->  c:/xxx[some new].abc
	inline std::string append_stem(const std::string & filename, const std::string & append)
	{
		namespace fs = std::filesystem;
		fs::path p{filename};
		p.replace_filename(p.stem().string() + append + p.extension().string());
		return p.string();
	}

	//获取扩展名     xxx.abc ->  .abc
	inline std::string get_extension(const std::string & filename)
	{
		namespace fs = std::filesystem;
		fs::path p{filename};
		return p.extension().string();
	}

	//替换扩展名: abc.xxx -> abc.you_want
	//extension:    e.g. ".txt"
	inline void replace_extension(std::string & result, const std::string & filename, const std::string & extension)
	{
		namespace fs = std::filesystem;
		fs::path path{filename};
		path.replace_extension(extension);
		result = path.string();
	}
	//特化
	inline void replace_extension(std::string & filename, const std::string & extension)
	{
		replace_extension(filename, filename, extension);
	}
}