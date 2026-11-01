#pragma once

#include <windows.h>
#include <string>
#include <stdint.h>
#include <utils/assert.hpp>

//Windows EventLog
namespace utils::windows::log
{
	enum class Level
	{
		information,
		warning,
		error,
	};
	//写入一条消息
	//note: 如果source不存在，并不报错，所以需要手动确保已经存在
	inline void write(const std::string & source, Level level, uint16_t category, uint32_t event_id, const std::string & text)
	{
		HANDLE hEventLog = RegisterEventSourceA(NULL, source.c_str()); utils_assert(hEventLog!=NULL);
		WORD wType = [&]() { switch(level) {
			case Level::information: return EVENTLOG_INFORMATION_TYPE;
			case Level::warning: return EVENTLOG_WARNING_TYPE;
			case Level::error: return EVENTLOG_ERROR_TYPE;
			}utils_throw();
		}();
		LPCSTR lpStrings[1] = {text.c_str()};
		utils_assert(ReportEventA(hEventLog,wType,category,event_id,NULL,1,0,lpStrings,NULL));
		utils_assert(DeregisterEventSource(hEventLog));
	}
}
