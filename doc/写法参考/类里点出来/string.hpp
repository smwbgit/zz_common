#pragma once

#include <string>

namespace utils::text
{
	namespace inner
	{
		struct s_compare;
		struct s_replace;
		struct s_convert;
		struct s_format;
	}

	class string
	{
	public:
		std::u16string m_text;
	public:
		string()
		{
		}
		string(const char16_t* text)
			: m_text{text}
		{
		}
		string(const std::u16string & o)
			: m_text{o}
		{
		}

		inner::s_compare & compare = *(inner::s_compare*)this;
		inner::s_replace & replace = *(inner::s_replace*)this;
		inner::s_convert & convert = *(inner::s_convert*)this;
		inner::s_format & format = *(inner::s_format*)this;
	};
	
	namespace inner
	{
		struct s_basic
		{
		protected:
			using string = utils::text::string;
		protected:
			const string & m_this() const
			{
				return *(const string*)this;
			}
			string & m_this()
			{
				return *(string*)this;
			}
		};
		//比较
		struct s_compare : public s_basic
		{
			//是否相同: 不区分大小写
			bool iequal(const string & o) const
			{
				return 0 == _wcsicmp((const wchar_t*)m_this().m_text.c_str(), (const wchar_t*)o.m_text.c_str());
			}
		};
		//替换
		struct s_replace : public s_basic
		{
			//全部替换
			void all(const string & from, const string & to)
			{
				//from如果为空，就直接返回
				if (from.m_text.empty())
					return;

				string finish;

				std::size_t begin_pos = 0;
				const auto & m_text = m_this().m_text;
				while (1)
				{
					std::size_t pos = m_text.find(from.m_text, begin_pos);
					if (pos == m_text.npos)
					{
						finish.m_text += m_text.substr(begin_pos);
						break;
					}

					finish.m_text += m_text.substr(begin_pos, pos - begin_pos);
					finish.m_text += to.m_text;
					begin_pos = pos + from.m_text.length();
				}

				m_this().m_text = std::move(finish.m_text);
			}
		};
		//转换
		struct s_convert : public s_basic
		{
			std::wstring wstring() const
			{
				return (const wchar_t*)m_this().m_text.c_str();
			}
		};
		//格式化
		struct s_format : public s_basic
		{
			//自己的格式
			//note:
			//		<0>: 第一个参数
			//		<1>: 第二个参数
			utils::text::string & simple(const utils::text::string & format, const utils::text::string & arg_0)
			{
				//todo 变为支持其他的的

				format.replace.all(u"<0>", arg_0);

				return m_this();
			}

			//c语言格式
			utils::text::string & sprintf(const char16_t* format, ...)
			{
				std::string text;
				va_list args;
				va_start(args, format);
				{
					int _length{};
					{
						va_list args1;
						va_copy(args1, args);
						_length = _vsnwprintf(nullptr, 0, (const wchar_t*)format, args1);
						va_end(args1);

						//简单检查
						utils_assert(_length >= 0);
					}

					va_list args2;
					va_copy(args2, args);	//一个va_list只能用一遍，再用要用va_start()或va_copy()重新创建
					m_this().m_text.resize(_length);
					int re = _vsnwprintf((wchar_t*)m_this().m_text.c_str(), m_this().m_text.length() + 1, (const wchar_t*)format, args2);
					va_end(args2);

					//简单检查
					utils_assert(re == m_this().m_text.length());
				}
				va_end(args);
				return m_this();
			}
		};
	}
}
