#pragma once

#include <string>
#include <utils/text/format.hpp>

//用人类易读的方式表示数据大小
namespace utils::text::friendly
{
	inline std::u8string data_size(uint64_t size)
	{
		if(size < 1024)
			return utils::text::format::format(u8"%llu B ", size);
		else if(size < 1024*1024)
			return utils::text::format::format(u8"%.1lf KB", size/1024.);
		else if(size < 1024*1024*1024)
			return utils::text::format::format(u8"%.1lf MB", size/1024./1024.);
		else
			return utils::text::format::format(u8"%.1lf GB", size/1024./1024./1024.);
	}
}