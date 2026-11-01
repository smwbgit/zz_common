#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "utils/assert.hpp"

namespace utils::windows::api::dropfile
{
	//启用拖拽
	inline void enable(HWND hwnd)
	{
		::DragAcceptFiles(hwnd, TRUE);
	}

	//获得拖拽过来的所有文件名
	inline void get_filenames(_Out_ std::vector<std::string> & filenames, _In_ const HDROP hDropInfo)
	{
#ifdef UNICODE
#error 不支持unicode版本
#endif
		filenames.clear();

		UINT file_n = ::DragQueryFile(hDropInfo, (UINT) -1, NULL, 0);
		utils_assert(file_n > 0);
		for (UINT i = 0; i < file_n; i++)
		{
			std::string filename;
			{
				//创建空间
				UINT length = DragQueryFile(hDropInfo, i, NULL, 0);
				utils_assert(length > 0);
				filename.resize(length);

	//			TCHAR szFileName[_MAX_PATH]{};
				auto _length = ::DragQueryFile(hDropInfo, i, (LPSTR)filename.data(), length+1);
				utils_assert(_length == length);
			}
			filenames.push_back(filename);
		}
	}
}