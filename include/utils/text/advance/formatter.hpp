#pragma once

#include <string>
#include <stdarg.h>
#include <utils/text/string.hpp>

//格式化
namespace utils::text::advance::formatter
{
	namespace inner::simple
	{
		class s_format
		{
		public:
			class s_expand
			{
			protected:
				utils::text::string m_text;
			public:
				s_format end()
				{
					return {std::move(m_text)};
				}
			protected:
				void init()
				{
				}
			private:
				friend class s_format;
			};
		private:
			utils::text::string m_text;
		public:
			s_format(const utils::text::string & text)
				: m_text{text}
			{
			}
			utils::text::string end()
			{
				return std::move(m_text);
			}
			s_format & arg(uint64_t index, const utils::text::string & text)
			{
				utils_assert(m_text.replace_all(utils::text::string().format(u8"<%llu>", index), text) > 0);
				return *this;
			}
			template<typename T>
			T expand()
			{
				T t{};
				t.m_text = std::move(m_text);
				t.init();
				return t;
			}
		};
	}

	//自己的格式
	inline inner::simple::s_format simple(const utils::text::string & format)
	{
		return {format};
	}
}