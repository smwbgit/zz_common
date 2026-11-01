#pragma once

#include <string>
#include <utils/text/time/filename.hpp>

namespace utils::text::filename::save_screenshot
{
	inline std::u8string make_by_dirname(const std::u8string & dir_name)
	{
		const auto time_text = utils::text::time::filename::get_text();
		return dir_name + u8"\\" + time_text + u8".bmp";
	}
}