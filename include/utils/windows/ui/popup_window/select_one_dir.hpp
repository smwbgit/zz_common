#pragma once


#include <string>
//#include "utils/standard_library_support/filesystem.hpp"


#include <Shlobj.h>
#pragma comment(lib, "Shell32.lib")


namespace utils::windows::ui
{
	//弹出一个窗口，让用户选择一个目录
	//返回值: 用户是选择了一个还是取消了
	//dirname: 返回值==true时，返回目录名
	//parent: 父窗口
	inline bool select_one_dir(std::string & dirname, HWND parent)
	{
		BROWSEINFOA bInfo{};
		bInfo.hwndOwner = parent;
		bInfo.lpszTitle = "浏览文件夹";
		bInfo.ulFlags = BIF_RETURNONLYFSDIRS
			| BIF_USENEWUI
			| BIF_NONEWFOLDERBUTTON;
		LPITEMIDLIST lpDlist = SHBrowseForFolderA(&bInfo);
		if (lpDlist)
		{
			char szPathName[MAX_PATH]{};
			SHGetPathFromIDListA(lpDlist,szPathName);	//todo: check the return value?
			dirname = szPathName;
			return true;
		}
		else
		{
			return false;
		}
	}
}

#if 0

//#error  选择文件能选择，但滚动不到那里。使用v1

namespace utils
{
	namespace v2
	{
		namespace windows
		{
			namespace inner
			{
				inline int CALLBACK browse_folder_callback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
				{
					if (uMsg == BFFM_INITIALIZED)
					{
						auto initial_folder = reinterpret_cast<std::string*>(lpData);
						::PostMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, (LPARAM) initial_folder->c_str());
					}
					return 0;
				}
			}

			//弹出一个窗口，让用户选择一个目录
			//is_selected: 用户是选择了一个还是取消了
			//dirname: is_selected==true时，返回目录名
			//parent: 父窗口
			//initial_folder: 开始时打开的目录
			inline bool select_one_dir(bool & is_selected, std::string & dirname, HWND parent, const std::string & initial_folder = "")
			{
				BROWSEINFOA bInfo = {0};
				bInfo.hwndOwner = parent;
				bInfo.lpszTitle = "浏览文件夹";
				bInfo.ulFlags = BIF_RETURNONLYFSDIRS
					| BIF_NEWDIALOGSTYLE
//					| BIF_NONEWFOLDERBUTTON
					| BIF_EDITBOX;
				if (false == initial_folder.empty())
				{
					bInfo.lParam = reinterpret_cast<LPARAM>(&initial_folder); 
					bInfo.lpfn = inner::browse_folder_callback;
				}
				LPITEMIDLIST lpDlist = SHBrowseForFolderA(&bInfo);
				if (lpDlist)
				{
					is_selected = true;
					char szPathName[MAX_PATH]{};
					SHGetPathFromIDListA(lpDlist,szPathName);	//todo: check the return value?
					dirname = szPathName;
				}
				else
				{
					is_selected = false;
				}
				return true;
			}
		}
	}
}
#endif