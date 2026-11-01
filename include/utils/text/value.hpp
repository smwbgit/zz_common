#pragma once

#include <string>
#include <map>
#include <utils/type_traits.hpp>
#include <utils/text/format.hpp>
#include <utils/basic.hpp>
#include <utils/text/quote.hpp>

//文本::值
namespace utils::text::value
{
	namespace inner
	{
#pragma region s_from
		template<typename T, class T_enable = void>
		struct s_from
		{
		};
		//特化
		template<>
		struct s_from<bool>
		{
			static void from_text(_Out_ bool & value, _In_ const std::string & text)
			{
				if (text == "true")
					value = true;
				else if (text == "false")
					value = false;
				else
					utils_throw();
			}
		};
		//特化
		template<>
		struct s_from<float>
		{
			static void from_text(_Out_ float & value, _In_ const std::string & text)
			{
				utils_std_exception_not_throwed(value = std::stof(text));	//todo 看看to_text<float> 再from_text<float> 是否可以准确获得原来的值
			}
		};
		//特化
		template<>
		struct s_from<double>
		{
			static void from_text(_Out_ double & value, _In_ const std::string & text)
			{
				utils_std_exception_not_throwed(value = std::stod(text));
			}
		};
		//特化
		template<>
		struct s_from<uint64_t>
		{
			static void from_text(_Out_ uint64_t & value, _In_ const std::string & text, uint64_t base = 10)
			{
				size_t pos{};
				utils_std_exception_not_throwed(value = std::stoull(text, &pos, (int)base));

				//简单检查
				utils_assert(pos == text.length());
			}
		};
		//特化
		template<>
		struct s_from<int64_t>
		{
			static void from_text(_Out_ int64_t & value, _In_ const std::string & text, uint64_t base = 10)
			{
				size_t pos{};
				utils_std_exception_not_throwed(value = std::stoll(text, &pos, (int)base));

				//简单检查
				utils_assert(pos == text.length());
			}
		};
		//特化
		template<typename T>
		struct s_from<T, std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>>>
		{
			static void from_text(_Out_ T & value, _In_ const std::string & text, uint64_t base = 10)
			{
				//转换
				uint64_t _value{};
				s_from<uint64_t>::from_text(_value, text, base);

				//保存
				utils::basic::store_in(value, _value);
			}
		};
		//特化
		template<typename T>
		struct s_from<T, std::enable_if_t<std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>>>
		{
			static void from_text(_Out_ T & value, _In_ const std::string & text, uint64_t base = 10)
			{
				//转换
				int64_t _value{};
				s_from<int64_t>::from_text(_value, text, base);

				//保存
				utils::basic::store_in(value, _value);
			}
		};
		//特化
		template<>
		struct s_from<char>
		{
			static void from_text(_Out_ char & value, _In_ const std::string & text)
			{
				//简单检查
				utils_assert(text.empty() == false);

				//判断表示方式
				if (text[0] == '\'')
				{
					//引号方式

					if (text[1] == '\\' && text[2] != '\'')
					{
						//转义表示

						//简单检查
						utils_assert(text.size() == 4);
						utils_assert(text[3] == '\'');

						//给出
						switch (text[2])
						{
						case 't':
							value = '\t';
							break;
						case 'n':
							value = '\n';
							break;
						default:
							utils_throw();
						}
					}
					else
					{
						//普通表示

						//简单检查
						utils_assert(text.size() == 3);
						utils_assert(text[2] == '\'');

						//给出
						value = text[1];
					}
				}
				else
				{
					//数字方式

					//转换
					uint8_t c{};
					s_from<uint8_t>::from_text(c, text);

					//给出
					value = c;
				}
			}
		};
		//特化
		template<>
		struct s_from<std::string>
		{
			static void from_text(_Out_ std::string & value, _In_ const std::string & text)
			{
				value = utils::text::quote::from_quoted(text);
			}
			static void to_text(_Out_ std::string & text, _In_ const std::string & value)
			{
				text = utils::text::quote::to_quoted(value);
			}
		};
#pragma endregion

#pragma region s_to
		template<typename T, class T_enable = void>
		struct s_to
		{
		};
		//特化
		template<>
		struct s_to<bool>
		{
			static void to_text(_Out_ std::string & text, _In_ const bool & value)
			{
				text = value ? "true" : "false";
			}
		};
		//特化
		template<typename T>
		struct s_to<T, std::enable_if_t<
			std::is_same_v<T, float> ||
			std::is_same_v<T, uint8_t> ||
			std::is_same_v<T, uint16_t> ||
			std::is_same_v<T, uint32_t> ||
			std::is_same_v<T, uint64_t> ||
			std::is_same_v<T, int8_t> ||
			std::is_same_v<T, int16_t> ||
			std::is_same_v<T, int32_t> ||
			std::is_same_v<T, int64_t>
			>>
		{
			static void to_text(_Out_ std::string & text, _In_ const T & value)
			{
				text = std::to_string(value);
			}
		};
		//特化
		template<>
		struct s_to<double>
		{
			static void to_text(_Out_ std::u8string & text, _In_ const double & value)
			{
				text = utils::text::format::format(u8"%.18lg", value);	//todo 看看有没有什么办法，又能简洁，又能保持原有值
			}
		};
		//特化
		template<>
		struct s_to<char>
		{
			//example:
            //			' '
            //			'''
            //			'"'
            //			'\'
            //			'\n'
            //			'\t'
			static void to_text(_Out_ std::string & text, _In_ const char & value)
			{
				//如果是普通字符
				if ((uint8_t)value < 0x80 && isprint(value))	//防止有的locale会把'\xa0'当作可打印字符
				{
					//单引号引起来，然后给出

					text = '\'';
					text += value;
					text += '\'';
				}
				//如果可转义表示
				else if(value == '\t' || value == '\n')
				{
					//用 单引号+转义表示 给出

					static std::map<char, std::string> table{
						{'\t', "'\\t'"},
						{'\n', "'\\n'"},
					};
					text = table[value];
				}
				//否则
				else
				{
					//用数字给出

					s_to<uint8_t>::to_text(text, (uint8_t)value);		//todo 看看用不用十六进制表示
				}
			}
		};
		//特化
		template<>
		struct s_to<std::string>
		{
			static void to_text(_Out_ std::string & text, _In_ const std::string & value)
			{
				text = utils::text::quote::to_quoted(value);
			}
		};
#pragma endregion
	}
	//从文本转为值
	template<typename T>
	inline void from_text(_Out_ T & value, _In_ const std::string & text)
	{
		inner::s_from<T>::from_text(value, text);
	}
	//特化: 进制
	template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
	inline void from_text(_Out_ T & value, _In_ const std::string & text, _In_ uint64_t base)
	{
		inner::s_from<T>::from_text(value, text, base);
	}
	//从值转为文本
	template<typename T>
	inline void to_text(_Out_ std::string & text, _In_ const T & value)
	{
		inner::s_to<T>::to_text(text, value);
	}
}
