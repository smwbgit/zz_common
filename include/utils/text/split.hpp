#pragma once

#include <string>
#include <functional>
#include <utils/integer_type.hpp>
#include <utils/assert.hpp>

namespace utils::text::split
{
	template<typename string_T = std::u8string>
	class RealImpl
	{
	public:
		using char_T = typename string_T::value_type;
	public:
		std::function<void(_In_ const string_T & text, _In_ u8 i, _Out_ u8 & separator_pos, _Out_ u8 & separator_size)> m_get_separator_pos;
		std::function<void(_In_ const string_T & word)> m_meet_one_word;
		bool m_is_keep_empty = true;
	private:
		const string_T & m_text;
		std::vector<string_T>* m_result_array{};
	public:
		RealImpl(const string_T & text)
			: m_text{text}
		{
		}
		void execute()
		{
			//简单检查
			utils_assert(m_get_separator_pos);
			utils_assert(
				m_meet_one_word && !m_result_array ||
				!m_meet_one_word && m_result_array);

			if (m_text.empty())
			{
				meet_one_word({});
				return;
			}

			for (u8 i = 0;;)
			{
				u8 separator_pos{};
				u8 separator_size{};
				m_get_separator_pos(m_text, i, separator_pos, separator_size);

				//简单检查
				utils_assert(separator_pos >= i);
				utils_assert(separator_pos <= m_text.size());
				utils_assert(separator_pos + separator_size <= m_text.size());

				if(separator_pos == m_text.size())
				{
					meet_one_word(m_text.substr(i));
					return;
				}
				
				meet_one_word(m_text.substr(i, separator_pos - i));
				i = separator_pos + separator_size;

				if(i == m_text.size())
				{
					meet_one_word({});
					return;
				}
			}
		}
	public:
		void set_separator(const string_T & separator)
		{
			//简单检查
			utils_assert(!m_get_separator_pos);

			auto _separator = separator;	//为了能处理类似set_separator(some_object.get_string())的情况
			m_get_separator_pos = [_separator](_In_ const string_T & text, _In_ u8 i, _Out_ u8 & separator_pos, _Out_ u8 & separator_size)
			{
				separator_pos = text.find(_separator, i);
				if (separator_pos == text.npos)
				{
					separator_pos = text.size();
					separator_size = 0;
					return;
				}
				separator_size = _separator.size();
			};
		}
		void set_separator_char(std::function<bool(char_T c)> is_separator_char)
		{
			//简单检查
			utils_assert(!m_get_separator_pos);

			m_get_separator_pos = [is_separator_char](_In_ const string_T & text, _In_ u8 i, _Out_ u8 & separator_pos, _Out_ u8 & separator_size)
			{
				auto find_separator_begin = [&]()
				{
					separator_pos = text.size();
					for(u8 j = i; j < text.size(); j++)
					{
						if(is_separator_char(text[j]))
						{
							separator_pos = j;
							break;
						}
					}
				};
				auto find_separator_size = [&]()
				{
					separator_size = text.size()-separator_pos;
					for(u8 j = separator_pos; j < text.size(); j++)
					{
						if(!is_separator_char(text[j]))
						{
							separator_size = j - separator_pos;
							break;
						}
					}
				};
				find_separator_begin();
				find_separator_size();
			};
		}
		void set_result_array(std::vector<string_T> & array)
		{
			//简单检查
			utils_assert(!m_result_array);

			m_result_array = &array;
		}
	private:
		void meet_one_word(const string_T & word)
		{
			//跳过空值
			if (false == m_is_keep_empty) if (word.empty()) return;

			if(m_meet_one_word)
				m_meet_one_word(word);
			else
				m_result_array->push_back(word);
		};
	};
}