#pragma once

#include <time.h>
#include <utils/assert.hpp>

namespace utils::time::c_time
{
	//对time()的简单包装
	inline time_t time()
	{
		time_t t = ::time(0);

		//简单检查
		utils_assert(t != (time_t)-1);

		return t;
	}
	//对localtime_s()的简单包装
	inline tm localtime_s(time_t t)
	{
		tm _tm{};
		utils_assert(0 == ::localtime_s(&_tm, &t));
		return _tm;
	}
	/* example:
		auto _tm = utils::time::c_time::get_tm();
		auto text = utils::text::format::format(
			u8"%02d%02d%02d_%02d%02d%02d",
			(_tm.tm_year + 1900)%100,
			_tm.tm_mon + 1,
			_tm.tm_mday,
			_tm.tm_hour,
			_tm.tm_min,
			_tm.tm_sec);
	*/
	inline tm get_tm()
	{
		time_t t = utils::time::c_time::time();
		return utils::time::c_time::localtime_s(t);
	}
}