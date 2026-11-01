#pragma once

#include <any>
#include <string>
#include "utils/map.hpp"
#include "utils/assert.hpp"
#include "utils/string.hpp"

namespace utils
{
	namespace inner
	{
		class s_value 
		{
		private:
			std::any m_value;
		public:
//断言类型和之前的相同
#define assert_type_same(_type) utils_assert(m_value.has_value() == false || m_value.type() == typeid(_type))


			//无符号整数
			template<typename T, std::enable_if_t<std::is_integral_v<T> &&std::is_unsigned_v<T>>>
			s_value& operator = (const T value)
			{
				//简单检查
				assert_type_same(uint64_t);

				m_value = (uint64_t)value;
				return *this;
			}
			//有符号整数
			template<typename T, std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
			s_value& operator = (const T value)
			{
				//简单检查
				assert_type_same(int64_t);

				m_value = (int64_t)value;
				return *this;
			}
			//浮点数
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, float> || std::is_same_v<T, double>>>
			s_value& operator = (const T value)
			{
				//简单检查
				assert_type_same(T);

				m_value = value;
				return *this;
			}
			//字符串
			template<typename T, std::enable_if_t<std::is_same_v<T, const char*> || std::is_same_v<T, std::string>>>
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

			//无符号整数
			template<typename T, std::enable_if_t<std::is_integral_v<T>&& std::is_unsigned_v<T>>>
			operator T() const
			{
				//简单检查
				assert_type_is(uint64_t);

				auto _value = std::any_cast<uint64_t>(m_value);
				utils::	//todo: 检查T类型是否能放下
				return (T);
			}

			//有符号整数
			template<typename T, std::enable_if_t<std::is_integral_v<T>&& std::is_signed_v<T>>>
			operator T() const
			{
				//简单检查
				assert_type_is(int64_t);

				return (T)std::any_cast<int64_t>(m_value);
			}

			//浮点数
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, float> || std::is_same_v<T, double>>>
			operator T() const
			{
				//简单检查
				assert_type_is(T);

				return std::any_cast<T>(m_value);
			}

			//浮点数
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, float> || std::is_same_v<T, double>>>
			operator T() const
			{
				//简单检查
				assert_type_is(T);

				return std::any_cast<T>(m_value);
			}

#undef assert_type_is
		};
	}

	//用字符串来索引值的配置表
	class Setting
	{
	private:
		utils::map<utils::string, inner::s_value> m_map;

	public:
		inner::s_value & operator[](const std::string & name)
		{
			return m_map[name];
		}
	};
}