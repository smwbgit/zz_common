#pragma once

#include <time.h>
#include <utils/text/string.hpp>
#include <utils/text/format.hpp>

namespace utils::text::date
{
#if 0	//请使用：utils::text::friendly::time::for_filename::get_text()
	//返回当前时间字符串  e.g. 20240215-130511-777
	inline utils::text::string current()
	{
		time_t t = time(0);
		tm _tm{};
		utils_assert(0 == localtime_s(&_tm, &t));

		return utils::text::string{}.format_sprintf(u"%d%02d%02d-%02d%02d%02d",
			_tm.tm_year + 1900,
			_tm.tm_mon + 1,
			_tm.tm_mday,
			_tm.tm_hour,
			_tm.tm_min,
			_tm.tm_sec);
	}
#endif

	//note: 可用的替代字段:
	//		<time>:  20240215-130511-777
	class format_simple : public utils::text::format::inner::simple::s_format::s_expand
	{
	public:
		void init()
		{
			m_text.replace_all(u"<time>", utils::text::date::current());
		}
	};
}
