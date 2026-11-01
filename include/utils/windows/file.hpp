#pragma once

#include <windows.h>
#include <utils/text/encoding.hpp>

namespace utils::windows::file
{
	//获取文件的完整路径
	inline std::u8string get_full_name_by_hFile(HANDLE hFile)
	{
		//由ai生成后修改

		DWORD dwFlags = VOLUME_NAME_DOS;
		DWORD dwSize = GetFinalPathNameByHandleW(hFile, NULL, 0, dwFlags);
		utils_assert(ERROR_NOT_ENOUGH_MEMORY == GetLastError());
		utils_assert(dwSize >= 1);

		// 分配足够的缓冲区
		std::u16string fullname_u16;
		fullname_u16.resize(dwSize - 1);

		// 获取文件的完整路径
		dwSize = GetFinalPathNameByHandleW(hFile, (wchar_t*)fullname_u16.c_str(), dwSize, dwFlags);
		utils_assert(ERROR_SUCCESS == GetLastError());
		utils_assert(dwSize == fullname_u16.length());

		//去掉开头的//?/
		auto fullname_u8 = utils::text::encoding::unicode::utf16_to_utf8(fullname_u16);
		utils_assert(fullname_u8.substr(0,4) == u8"\\\\?\\");
		fullname_u8 = fullname_u8.substr(4);
		utils_assert(fullname_u8.length() < 250);	//确保这不是个长路径	//todo 看看250这里具体是多少

		return fullname_u8;
	}
}