#pragma once

#include <utils/assert.hpp>

namespace utils::windows::exception
{
	//捕捉内存访问异常
	//return: 捕捉到, 返回true
	inline bool catch_memory_access_exception(std::function<void()> func)
	{
		try
		{
			func();
			return false;
		}
		catch(...) 
		{
			return true;
		}
	}
}