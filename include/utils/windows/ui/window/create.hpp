#pragma once

#include <windows.h>
#include <utils/windows/ui/window/ObjectBinder.hpp>
#include <utils/windows/ui/window/message_loop.hpp>
#include <utils/text/encoding.hpp>
#include <utils/this_thread.hpp>

namespace utils::windows::ui::window::create
{
	namespace user_message
	{
		inline const uint64_t UM_CREATE = WM_USER;
	}

	//创建一个窗口，此窗口的消息循环在一个新的单独线程中
	//T: 窗口类型
	template<typename T, typename... T_args>
	void in_new_thread(const std::u8string & class_name, DWORD dwExStyle, DWORD dwStyle,
		int x, int y, int width, int height, const std::u8string & title, T_args && ... args)
	{
		std::atomic_bool is_ready = false;

		std::thread{[&]()
		{
			const auto class_name_u16 = utils::text::encoding::utf8_to_utf16(class_name);
			const wchar_t* CLASS_NAME = (const wchar_t*)class_name_u16.c_str();
			auto title_u16 = utils::text::encoding::utf8_to_utf16(title);

			WNDCLASSW wc {};
			wc.cbWndExtra = sizeof(int*);
			wc.lpfnWndProc = utils::windows::ui::window::ObjectBinder<T>::WindowProc;
			wc.hInstance = GetModuleHandleA(0);
			wc.lpszClassName = CLASS_NAME;
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);

			RegisterClassW(&wc);

			HWND hwnd = CreateWindowExW(
				dwExStyle,
				CLASS_NAME,                     // Window class
				(const wchar_t*)title_u16.c_str(),    // Window text
				dwStyle,            // Window style

				// Size and position
				x, y, width, height,

				NULL,       // Parent window    
				NULL,       // Menu
				GetModuleHandleA(0),  // Instance handle
				NULL        // Additional application data
				);

			utils_assert(hwnd);

			T & wnd = utils::windows::ui::window::ObjectBinder<T>::get_object(hwnd);
			wnd.set_args(std::forward<T_args>(args)...);
			is_ready = true;
		
			SendMessageA(hwnd, utils::windows::ui::window::create::user_message::UM_CREATE, 0, 0);
			ShowWindow(hwnd, SW_SHOWNOACTIVATE);
			UpdateWindow(hwnd);

			//todo: 考虑在消息循环中，检查窗口个数，当为0时，结束线程。以免窗口类中忘记调用PostQuitMessage().看看enumwindow 是否适用
			utils::windows::ui::window::message_loop::MessageLoop message_loop;
			message_loop.start();
		}}.detach();
		
		utils::this_thread::wait_ready(is_ready);
	}
}