#pragma once

#include <windows.h>
#include <string>
#include <utils/assert.hpp>
#include <utils/real_path.hpp>

//消息钩子
namespace utils::windows::hook::message_hook
{
	//安装：向一个窗口安装
	//type: example: WH_CBT
	inline HHOOK install(int type, const std::string & filename, const std::string & func_name, HWND hwnd)
	{
		auto _filename = utils::real_path::get_absolute(filename);
		auto dll = LoadLibraryA(_filename.c_str());						utils_assert(dll);
		auto func = (HOOKPROC)GetProcAddress(dll, func_name.c_str());	utils_assert(func);
		auto tid = GetWindowThreadProcessId(hwnd, 0);					utils_assert(tid);
		auto hhook = SetWindowsHookExA(type, func, dll, tid);			utils_assert(hhook);
		return hhook;

		//utils_assert(UnhookWindowsHookEx(g_hook));
	}
}