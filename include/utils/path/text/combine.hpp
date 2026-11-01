#pragma once

#include <utils/standard_library_support/filesystem.hpp>
#include <string>
#include <vector>

namespace utils::path::text
{
	// 把目录和一个相对路径拼在一起
	//dirname: 目录						e.g.  c:/abc   or  c:/abc/   or ./abc  or ./abc/
	//relative_filename: 相对路径		e.g.  1.txt
	inline void combine(_Out_ std::filesystem::path & combined, _In_ const std::filesystem::path & dirname, _In_ const std::filesystem::path & relative_filename)
	{
		combined = dirname / relative_filename;
	}
	//特化
	inline void combine(_Out_ std::u8string & combined, _In_ const std::filesystem::path & dirname, _In_ const std::filesystem::path & relative_filename)
	{
		std::filesystem::path p;
		combine(p, dirname, relative_filename);
		combined = p.u8string();
	}
}