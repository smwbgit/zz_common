#pragma once

#include <chrono>
#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/assert.hpp>

//boost::posix_time库的帮助函数
namespace utils::namespace_boost::posix_time
{
	//从c++11标准形式转为当前形式
	template<typename Rep_T, typename Period_T>
	void convert(_Out_ boost::posix_time::time_duration & to, _In_ const std::chrono::duration<Rep_T, Period_T> & from)
	{
		//posix_time里的纳秒被注掉了, 就改用了微秒

		auto us = std::chrono::duration_cast<std::chrono::microseconds>(from).count();
		to = boost::posix_time::microseconds{us};
	}
}