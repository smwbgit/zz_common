#pragma once

#include <string>
#include <utils/time/c_time.hpp>
#include <utils/text/format.hpp>

namespace utils::text::time::filename
{
	inline std::u8string get_text()
	{
		const auto _tm = utils::time::c_time::get_tm();

		return utils::text::format::format(u8"%02d%02d%02d_%02d%02d%02d",
			(_tm.tm_year + 1900)%100,
			_tm.tm_mon + 1,
			_tm.tm_mday,
			_tm.tm_hour,
			_tm.tm_min,
			_tm.tm_sec);
	}
}