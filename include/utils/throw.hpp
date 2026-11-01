#pragma once

#include <exception>
#include <string>
#include <functional>
#include "wrap_statement.hpp"

namespace utils
{
	//utils库的异常根类
	class Exception : public std::exception
	{
	private:
		std::string m_msg;

	public:
		using std::exception::exception;
		Exception(const std::string & what)
			: m_msg {what}
		{
		}

	public:
		const char* what() const noexcept override
		{
			return m_msg.c_str();
		}
	};


	//判断一个函数是否抛出了utils::Exception
	inline bool throwed_utils_exception(std::function<void()> func)
	{
		try
		{
			func();
		}
		catch(utils::Exception & e [[maybe_unused]])
		{
			return true;
		}
		return false;
	}

	//判断一个函数是否抛出了std::exception
	inline bool throwed_std_exception(std::function<void()> func)
	{
		try
		{
			func();
		}
		catch(std::exception & e [[maybe_unused]])
		{
			return true;
		}
		return false;
	}

	namespace inner::Exception
	{
		inline std::string make_info(const char* filename, int line)
		{
			std::string text;
			text.resize(strlen(filename)+20);
			snprintf((char*)text.c_str(),text.length(),"%s(line:%d)", filename, line);
			return text.c_str();
		}
		inline std::string make_info(const char* filename, int line, const char* message)
		{
			std::string text;
			text.resize(strlen(filename)+30+strlen(message));
			snprintf((char*)text.c_str(),text.length(),"%s(line:%d): %s", filename, line, message);
			return text.c_str();
		}
	}
}

//抛出异常
#define utils_throw()  do {throw utils::Exception(utils::inner::Exception::make_info(__FILE__,__LINE__).c_str()); }while(0)
//特化
#define utils_throw_with_message(message)  do {throw utils::Exception(utils::inner::Exception::make_info(__FILE__,__LINE__,message).c_str()); }while(0)

//简便写法: 检查一行语句是否抛出了utils::Exception
#define utils_fail(statement)	(utils::throwed_utils_exception(utils_wrap_statement(statement)))

//添加try语义: 抛出异常返回false
#define utils_try(statement)	(false == utils_fail(statement))

//简便写法: 检查一行语句是否抛出了std::exception
#define utils_std_exception_not_throwed(statement)	utils_assert(false == utils::throwed_std_exception(utils_wrap_statement(statement)))


