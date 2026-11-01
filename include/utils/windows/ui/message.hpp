#pragma once

#include <windows.h>
#include <thread>
#include <utils/assert.hpp>
#include <utils/iterator.hpp>

#pragma comment(lib, "user32.lib")

namespace utils::windows::ui::message
{
	//向指定线程发送一个消息
	inline void post_message_to_thread(_In_ std::thread & thread, _In_ UINT msg_id, _In_ WPARAM wparam, _In_ LPARAM lparam)
	{
		auto tid = GetThreadId(thread.native_handle());
		utils_assert(tid);

		auto re = PostThreadMessageA(tid, msg_id, wparam, lparam);
		utils_assert(re);
	}
	//如果消息队列还没有创建，创建它
	inline void create_message_queue()
	{
		MSG msg{};
		PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	}

#if 0	//还未整理
	//当鼠标离开指定窗口时，向该窗口发送一个WM_MOUSELEAVE消息
	inline void track_mouse_leave(HWND hwnd)
	{
		TRACKMOUSEEVENT tme{};
		{
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
		}
		utils_assert(TrackMouseEvent(&tme));
	}
	//发送一系列消息, 以模拟单个键或组合键被按下
	//keys: 具体的按键, 可以是: "a"  "ctrl+b"  "ctrl+shift+c"  "ctrl+alt+shift+d" .
	//      说明: 字母需要放在最后
	inline void post_keyboard_message(_In_ HWND hwnd, _In_ std::string_view keys)
	{
		//确定所有需要按的键, 用VK_*表示
		std::vector<uint32_t> _keys;
		while (1)
		{
			//简单检查
			utils_assert(keys.empty() == false);

			if (keys.size() == 1)
			{
				char c = keys[0];

				//todo 添加更多的按键
				if (c >= 'a' && c <= 'z')
				{
					_keys.push_back(c - 'a' + 'A');
				}
				else if (c == '\n')
				{
					_keys.push_back(VK_RETURN);
				}
				else
					utils_throw();
				break;
			}

			if (keys.starts_with("ctrl+"))
			{
				keys.remove_prefix(5);

				_keys.push_back(VK_CONTROL);
				continue;
			}
			if (keys.starts_with("alt+"))
			{
				keys.remove_prefix(4);

				_keys.push_back(VK_MENU);
				continue;
			}
			if (keys.starts_with("shift+"))
			{
				keys.remove_prefix(6);
				
				_keys.push_back(VK_SHIFT);
				continue;
			}

			utils_throw();
		}

		//简单检查
		utils_assert(_keys.empty() == false);

		//todo:添加alt会影响消息类型和标志位的情况
		//依次按下,再依次抬起
		for (const auto key : _keys)
		{
			PostMessageA(hwnd, WM_KEYDOWN, key, 1);
		}
		for (auto i : utils::iterator_index(_keys))
		{
			const auto key = _keys[_keys.size() - i - 1];

			PostMessageA(hwnd, WM_KEYUP, key, 0xc0000001);
		}
	}
#endif
}