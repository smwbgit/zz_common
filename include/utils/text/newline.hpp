#pragma once

#include <string>

//换行符
namespace utils::text::newline
{
	inline const std::u8string CRLF = u8"\r\n";
	inline const std::u8string LF = u8"\n";
	inline const std::u8string windows_style = CRLF;
	inline const std::u8string linux_style = LF;
	inline const std::u8string _r_n = CRLF;
	inline const std::u8string _n = LF;
}