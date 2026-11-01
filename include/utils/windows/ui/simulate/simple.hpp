#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <utils/assert.hpp>
#include <utils/map.hpp>
#include <utils/Index2D.hpp>

//模拟鼠标和键盘: 以简单方式（而非内核方式）来实现
namespace utils::windows::ui::simulate::simple
{
}

//鼠标确实会移动
namespace utils::windows::ui::simulate::simple::real
{
}

//以keybd_event/mouse_event方式
namespace utils::windows::ui::simulate::simple::real::event
{
	namespace keyboard
	{
		//模拟键盘按下
		inline void press(BYTE vkey)
		{
			keybd_event(vkey, 0, 0, 0);
		}
		//模拟键盘松开
		inline void release(BYTE vkey)
		{
			keybd_event(vkey, 0, KEYEVENTF_KEYUP, 0);
		}
		//模拟键盘一次点击
		inline void type(BYTE vkey)
		{
			press(vkey);
			release(vkey);
		}
	}
	namespace mouse
	{
		//模拟鼠标移动
		inline void move(utils::Index2D pos)
		{
			SetCursorPos(pos.i, pos.j); Sleep(100);
		}
		//模拟鼠标点击
		inline void click(utils::Index2D pos)
		{
			mouse::move(pos);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); Sleep(100);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); Sleep(100);
		}
		//模拟鼠标右键点击
		inline void right_click(utils::Index2D pos)
		{
			mouse::move(pos);
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0); Sleep(100);
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0); Sleep(100);
		}
	}
}

//以SendInput方式
namespace utils::windows::ui::simulate::simple::real::send_input
{
	namespace inner
	{
		//发送数据
		inline void send_input(const INPUT & input)
		{
			//准备数据: input数组
			std::vector<INPUT> inputs{ input };

			//发送数据
			utils_assert(inputs.size() == SendInput(
				static_cast<UINT>(inputs.size()),
				inputs.data(),
				sizeof(INPUT)));

			//短暂等待一下
			Sleep(100);
		}
	}
	namespace keyboard
	{
		//模拟键盘按下
		inline void press(const BYTE vkey)
		{
			//note: ai生成，稍做修改

			//准备一个 INPUT 结构
			INPUT in{};
			in.type           = INPUT_KEYBOARD;
			in.ki.wVk         = vkey;
			in.ki.dwFlags     = 0;                    // 0 表示按下
			in.ki.time        = 0;
			in.ki.dwExtraInfo = 0;

			//发送数据
			inner::send_input(in);
		}
		//模拟键盘松开
		inline void release(const BYTE vkey)
		{
			//note: ai生成，稍做修改

			//准备一个 INPUT 结构
			INPUT in{};
			in.type           = INPUT_KEYBOARD;
			in.ki.wVk         = vkey;
			in.ki.dwFlags     = KEYEVENTF_KEYUP;      // 关键标志
			in.ki.time        = 0;
			in.ki.dwExtraInfo = 0;

			//发送数据
			inner::send_input(in);
		}
		//模拟键盘一次点击
		inline void type(const BYTE vkey)
		{
			press(vkey);
			release(vkey);
		}

		namespace 按键组合
		{
			inline void 全选并粘贴()
			{
				INPUT input[6]{};
				input[0].type = input[1].type = input[2].type = input[3].type =
					input[4].type = input[5].type = INPUT_KEYBOARD;
				input[0].ki.wVk = input[5].ki.wVk = VK_CONTROL;
				input[1].ki.wVk = input[2].ki.wVk = 'A';
				input[3].ki.wVk = input[4].ki.wVk = 'V';
				input[2].ki.dwFlags = input[4].ki.dwFlags = input[5].ki.dwFlags = KEYEVENTF_KEYUP;

				utils_assert(6 == SendInput(6, input, sizeof(INPUT)));
			}
		}
#if 0	//不知道这样是否能行
		inline void 让所有按键抬起()
		{
			for (int i = 0; i < 256; i++)
			{
				//使用VK_LSHIFT/VK_RSHIFT, 而不是VK_SHIFT
				if (i == VK_SHIFT || i == VK_CONTROL || i == VK_MENU)
					continue;

				auto state = GetAsyncKeyState(i);
				if (state & 0x8000)
				{
					INPUT input[1]{};
					input[0].type = INPUT_KEYBOARD;
					input[0].ki.wVk = i;
					input[0].ki.dwFlags = KEYEVENTF_KEYUP;
					utils_assert(1 == SendInput(1, input, sizeof(INPUT)));
				}
			}
		}
#endif
	}
	namespace mouse
	{
		//模拟鼠标移动
		inline void move(const utils::Index2D pos)
		{
			//准备一个 INPUT 结构
			INPUT in{};
			in.type           = INPUT_MOUSE;
			in.mi.dwFlags     = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
			in.mi.dx          = static_cast<LONG>(pos.i * 65535 / GetSystemMetrics(SM_CXSCREEN));
			in.mi.dy          = static_cast<LONG>(pos.j * 65535 / GetSystemMetrics(SM_CYSCREEN));

			//发送数据
			inner::send_input({in});
		}
		//模拟鼠标按下
		inline void press(const bool is_left_button = true)
		{
			//准备数据: 标志
			DWORD dwFlags = is_left_button ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;

			//发送数据: 鼠标按下
			inner::send_input(
				INPUT{.type = INPUT_MOUSE, .mi = {.dwFlags = dwFlags}}
			);
		}
		//模拟鼠标抬起
		inline void release(const bool is_left_button = true)
		{
			//准备数据: 标志
			DWORD dwFlags = is_left_button ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;

			//发送数据: 鼠标抬起
			inner::send_input(
				INPUT{.type = INPUT_MOUSE, .mi = {.dwFlags = dwFlags}}
			);
		}
		//模拟鼠标点击
		inline void click(const utils::Index2D position)
		{
			//移动鼠标到指定位置
			move(position);

			//鼠标按下
			press();

			//鼠标抬起
			release();
		}
		//模拟鼠标右键点击
		inline void right_click(const utils::Index2D position)
		{
			//移动鼠标到指定位置
			move(position);

			//鼠标右键按下
			press(false);

			//鼠标右键抬起
			release(false);
		}
		//模拟鼠标拖动
		inline void drag(const utils::Index2D from, const utils::Index2D to)
		{
			//移动鼠标到起始位置
			move(from);

			//鼠标按下
			press();

			//对于拖动，等待时间，再稍稍长点，不然有时，如果只等待100ms，拖动后，无效果
			Sleep(500);

			//移动鼠标到终点位置
			move(to);

			//鼠标抬起
			release();
		}
	}
}

//以PostMessage方式
namespace utils::windows::ui::simulate::simple::message
{
	//模拟鼠标点击
	/*
	note:
		此方法在倩女幽魂中，需要把鼠标实际移动到x,y点
		在画图中，不起作用
	*/
	inline void click(HWND hwnd, int x, int y)
	{
		POINTS pt{(SHORT)x,(SHORT)y};
		SendMessageA(hwnd, WM_MOUSEMOVE, 0, *(LPARAM*)&pt);
		SendMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, *(LPARAM*)&pt);
		SendMessageA(hwnd, WM_LBUTTONUP, 0, *(LPARAM*)&pt);
	}
	//特化
	inline void click(HWND hwnd, utils::Index2D pos)
	{
		return click(hwnd, pos.i, pos.j);
	}

	namespace keyboard
	{
		using vkey_t = UINT;	//todo 看看虚拟码用什么类型表示，是否是UINT

		//模拟键盘输入
		//key: 类似VK_F5的虚拟码
		inline void type(HWND hwnd, vkey_t key)
		{
			SendMessageA(hwnd, WM_KEYDOWN, key, 0);
			SendMessageA(hwnd, WM_KEYUP, key, 0);
		}
		//特化
		//example: type("F1");
		inline void type(HWND hwnd, const std::string & key)
		{
			static const std::map<std::string, vkey_t> table{
				{"F1",VK_F1},
				{"F2",VK_F2},
				{"F3",VK_F3},
				{"F4",VK_F4},
				{"F5",VK_F5},
				{"F6",VK_F6},
				{"F7",VK_F7},
				{"F8",VK_F8},
				{"F9",VK_F9},
				{"F10",VK_F10},
				{"F11",VK_F11},
				{"F12",VK_F12},
				{"tab",VK_TAB},
				{"b",'B'},
				{"o",'O'},
				{"p",'P'},
				{"y",'Y'},
			};

			return type(hwnd, utils::map(table).get_existed_value(key));
		}
	}

	using keyboard::type;
}