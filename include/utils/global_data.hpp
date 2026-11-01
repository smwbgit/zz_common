#pragma once

#include <map>
#include <any>
#include "utils/Singleton.hpp"

namespace utils
{
	namespace inner::global_data
	{
#if 0
		class s_value
		{
		private:
			std::any m_value;

		public:
			//断言类型和之前的相同
#define assert_type_same(_type) utils_assert(m_value.has_value() == false || m_value.type() == typeid(_type))

			//整数和浮点数: 存为double
			template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
			s_value& operator = (const T value)
			{
				//简单检查
				assert_type_same(double);

				m_value = (double)value;
				return *this;
			}
			//字符串
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, const char*> || std::is_same_v<T, std::string>>>
			s_value& operator = (const T & value)
			{
				//简单检查
				assert_type_same(std::string);

				m_value = (std::string)value;
				return *this;
			}

#undef assert_type_same

			//断言是这个类型
#define assert_type_is(_type) utils_assert(m_value.has_value() == true && m_value.type() == typeid(_type))

			//整数和浮点数
			template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
			operator T() const
			{
				//简单检查
				assert_type_is(double);

				auto _value = std::any_cast<double>(m_value);
				return (T)_value;
			}

			//字符串
			operator std::string() const
			{
				//简单检查
				assert_type_is(std::string);

				auto _value = std::any_cast<std::string>(m_value);
				return _value;
			}
			//字符串
			operator const char*() const
			{
				//简单检查
				assert_type_is(std::string);

				const auto & _value = std::any_cast<const std::string &>(m_value);
				return _value.c_str();
			}

#undef assert_type_is

			//是否有值
			bool has_value() const
			{
				return m_value.has_value();
			}
		};
#endif

		class s_value
		{
		private:
			std::any m_value;

		public:
			template<typename T>
			s_value& operator = (T && value)
			{
				//简单检查: 类型和之前的相同
				utils_assert(m_value.has_value() == false || m_value.type() == typeid(T));

				m_value.operator=(std::forward<T>(value));
				return *this;
			}

			template<typename T>
			operator T() const
			{
				//简单检查
				utils_assert(m_value.has_value() == true && m_value.type() == typeid(T));

				return std::any_cast<T>(m_value);
			}

			//是否有值
			bool has_value() const
			{
				return m_value.has_value();
			}
		};
		using GlobalData = std::map<std::string, utils::inner::global_data::s_value>;
	}
}

//一些可以在程序任何地方使用的数据
//example:
//    utils_global_data["ui/block/x"] = 1;
//    int x = utils_global_data["ui/block/x"];
#define utils_global_data (utils::Singleton<utils::inner::global_data::GlobalData>::get())
