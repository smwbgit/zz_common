#pragma once

#include <string>
#include <vector>
#include "utils/string.hpp"

namespace utils::literal
{
	//把一个十六进制表示的字符串转为字节数组. i.e.  "313233"_vb  ==  std::vector<uint8_t>{0x31, 0x32, 0x33}
	//note:	v表示vector; b表示uint8_t(i.e. byte)
	inline std::vector<uint8_t> operator"" _vb(const char* str, std::size_t size)
	{
		std::vector<uint8_t> data;
		utils::string_function::get_byte_array_from_hex_string(data, std::string{str, size});
		return data;
	}
}