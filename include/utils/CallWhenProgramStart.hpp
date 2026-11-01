#pragma once

#include <functional>

namespace utils::inner::CallWhenProgramStart
{
	class Helper
	{
	public:
		Helper(std::function<void()> func)
		{
			func();
		}
	};
}

//当程序开始运行时，运行指定的语句
//example:
//	CallWhenProgramStart([]()
//	{
//		utils::serialize::any::add_serializer<bool>();
//		utils::serialize::any::add_serializer<char>();
//	});
#define CallWhenProgramStart(statements)	\
static inline utils::inner::CallWhenProgramStart::Helper zz__CallWhenProgramStart{(statements)}
