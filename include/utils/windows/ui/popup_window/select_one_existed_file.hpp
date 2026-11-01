#pragma once


#include <string>
#include "utils/standard_library_support/filesystem.hpp"


#include <Shlobj.h>
#pragma comment(lib, "Shell32.lib")


namespace utils::windows::ui
{
	//弹出一个窗口，让用户选择一个已经存在的文件
	//返回值: 用户是选择了一个还是取消了
	//filename: 返回值==true时，返回文件位置
	//parent: 对话框的父窗口
	inline bool select_one_existed_file(std::string & filename, HWND parent)
	{
		char szPathName[MAX_PATH]{};  
		OPENFILENAMEA ofn{sizeof(ofn)};  
		ofn.hwndOwner = parent;
//		ofn.lpstrFilter = "All\0*.*\0";	//过滤器 如果为 NULL 不使用过滤器 
		ofn.lpstrFilter = 0;	//过滤器 如果为 NULL 不使用过滤器 
		ofn.lpstrFile = szPathName;
		ofn.nMaxFile = sizeof(szPathName);
		ofn.lpstrTitle = "选择文件";
		ofn.lpstrInitialDir=0;//设置对话框显示的初始目录  
//		TCHAR szCurDir[MAX_PATH];  
//		GetCurrentDirectory(sizeof(szCurDir),szCurDir);  
//		ofn.lpstrInitialDir=szCurDir;//设置对话框显示的初始目录  
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY;
		if (GetOpenFileNameA(&ofn) == FALSE)
			return false;

		filename = szPathName;
		return true;
	}
}

