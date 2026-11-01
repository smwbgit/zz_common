#pragma once

#include <string>
#include <string_view>

//引号引起来的
namespace utils::text::quote
{
	//获得一个引号引起来的字符串
	//e.g.    abc  ->  会获得 "abc"  (包括引号)
	//e.g.    ab"c  ->  会获得 "ab\"c"  (包括引号和斜杠)
	//quote: 引号
	//escape: 转义符号
	inline std::string to_quoted(_In_ const std::string & text, _In_ const std::string & quote = "\"", _In_ const std::string & escape = "\\")
	{
		std::string result;

		//添加引号
		result += quote;

		std::string_view view = text;
		while (1)
		{
			if (view.starts_with(quote))
			{
				//进行转义

				result += escape + quote;
				view.remove_prefix(quote.length());
			}
			else if (view.starts_with(escape))
			{
				//进行转义

				result += escape + escape;
				view.remove_prefix(escape.length());
			}
			else if (view.empty())
			{
				//全部完成

				break;
			}
			else
			{
				//普通字符

				result += view[0];
				view.remove_prefix(1);
			}
		}

		//添加引号
		result += quote;

		//给出
		return result;
	}
	//去掉引号, i.e. 引号引起来的字符串 转为 原来字符串
	//e.g.    "abc"  (包括引号)  ->  会获得 abc
	//e.g.    "ab\"c"  (包括引号和斜杠)  ->  会获得 ab"c
	//quote: 引号
	//escape: 转义符号
	inline std::string from_quoted(_In_ const std::string & text, _In_ const std::string & quote = "\"", _In_ const std::string & escape = "\\")
	{
		std::string_view view = text;

		//去掉开头和结尾的引号
		{
			//简单检查
			utils_assert(view.starts_with(quote));
			utils_assert(view.ends_with(quote));

			view.remove_prefix(quote.length());
			view.remove_suffix(quote.length());
		}

		std::string result;
		while (1)
		{
			if (view.starts_with(escape))
			{
				//去掉转义

				view.remove_prefix(escape.length());

				if (view.starts_with(quote))
				{
					//实际内容是quote

					view.remove_prefix(quote.length());
					result += quote;
				}
				else if (view.starts_with(escape))
				{
					//实际内容是escape

					view.remove_prefix(escape.length());
					result += escape;
				}
				else
				{
					utils_throw();
				}
			}
			else if (view.empty())
			{
				//全部完成

				break;
			}
			else
			{
				//普通字符

				result += view[0];
				view.remove_prefix(1);
			}
		}

		//给出
		return result;
	}
}
