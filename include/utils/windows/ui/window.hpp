#pragma once

#include <vector>
#include <windows.h>
#include <utils/Index2D.hpp>

namespace utils::windows::ui::window
{
	inline bool is_window_class_registered(const std::wstring & class_name)
	{
		HINSTANCE hInstance = GetModuleHandleA(NULL);
		WNDCLASS wc{};

		if(GetClassInfoW(hInstance, class_name.c_str(), &wc))
		{
			return true;
		}
		else
		{
			//简单检查
			utils_assert(ERROR_CLASS_DOES_NOT_EXIST == GetLastError());

			return false;
		}
	}
	//特化
	inline bool is_window_class_registered(const std::u8string & class_name)
	{
		const auto class_name_u16 = utils::text::encoding::utf8_to_utf16(class_name);

		//转发
		return is_window_class_registered((const std::wstring &)class_name_u16);
	}
	struct register_class__params
	{
		bool is_allow_registered{};	//是否允许class_name已经注册
		UINT style{};
		WNDPROC lpfnWndProc{};
		int cbClsExtra{};
		int cbWndExtra{};
		HINSTANCE hInstance{};
		HICON hIcon{};
		HCURSOR hCursor{};
		HBRUSH hbrBackground{};
		std::wstring menu_name;
		std::wstring class_name;

		register_class__params()
		{
			hInstance = GetModuleHandleA(NULL);
			hIcon = LoadIconW(nullptr, IDI_APPLICATION);
			hCursor = LoadCursorW(nullptr, IDC_ARROW);
			hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		}
	};
	//对RegisterClass的简单包装
	inline void register_class(const register_class__params & params)
	{
		if(params.is_allow_registered)
		{
			//已经注册
			if(is_window_class_registered(params.class_name))
				return;
		}

		WNDCLASSW wc{};
		wc.style = params.style;
		wc.lpfnWndProc = params.lpfnWndProc;
		wc.cbClsExtra = params.cbClsExtra;
		wc.cbWndExtra = params.cbWndExtra;
		wc.hInstance = params.hInstance;
		wc.hIcon = params.hIcon;
		wc.hCursor = params.hCursor;
		wc.hbrBackground = params.hbrBackground;
		wc.lpszMenuName = params.menu_name.empty() ? nullptr : params.menu_name.c_str();
		wc.lpszClassName = params.class_name.c_str();
		auto atom = RegisterClassW(&wc);
		utils_assert(atom != 0);
	}
	struct create_window__params
	{
		DWORD dwExStyle{};
		std::wstring class_name;
		std::wstring title;
		DWORD dwStyle{};
		int x{CW_USEDEFAULT};
		int y{CW_USEDEFAULT};
		int width{CW_USEDEFAULT};
		int height{CW_USEDEFAULT};
		HWND parent_hwnd{};
		HMENU menu{};
		HINSTANCE instance{};
		LPVOID lpParam{};

		create_window__params()
		{
			instance = GetModuleHandleA(NULL);
		}
	};
	//对CreateWindowEx的简单包装
	inline HWND create_window(const create_window__params & params)
	{
		HWND hwnd = CreateWindowExW(
			params.dwExStyle,
			params.class_name.c_str(),
			params.title.c_str(),
			params.dwStyle,
			params.x, params.y, params.width, params.height,
			params.parent_hwnd,
			params.menu,
			params.instance,
			params.lpParam
		);
		utils_assert(hwnd);
		return hwnd;
	}
}