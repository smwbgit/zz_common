#pragma once

#include "utils/data.hpp"
#include "utils/text/namespace.hpp"

//内存数据/二进制数据
namespace utils::text::memory
{
	//把二进制数据变为一段不换行的字符串
	//note: 不方便显示和不能显示的字符，由\xx表示。字符'\'由"\\"表示
	std::string to_text(_In_ const utils::data & data)
	{
		std::string text;
		for (const auto c : data.wrapped_type())
		{
			if (isalnum(c))
				text += c;
			else if (c == ' ')
				text += c;
			else if (c == '\\')
				text += "\\\\";
			else if (ispunct(c))
				text += c;
			else
				text += utils::text::format("\\%02x", c);
		}
		return text;
	}
}