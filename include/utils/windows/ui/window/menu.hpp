#pragma once

#include <windows.h>
#include <utils/windows/window/Window.hpp>

namespace utils::windows::window::menu
{
	//在鼠标位置弹出菜单
	//items: 所有菜单项
	//返回: 选择的菜单项 或者 "未选择"
	inline std::u16string popup(const std::vector<std::u16string> & items)
	{
		//弹出菜单需要一个父窗口
		class Window : public utils::windows::window::Window<Window>
		{
		private:
			using base_type = utils::windows::window::Window<Window>;
		public:
			static void get_dwStyle(WindowStyle & window_style, DWORD & dwStyle, DWORD & dwExStyle)
			{
				window_style = WindowStyle::透明窗口;
			}
		public:
			LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
			{
				switch (uMsg)
				{
				case WM_CREATE:
				{
					//把此窗口放到鼠标底下，以使得鼠标在被锁定到屏幕中心等情况时，能挪动鼠标选择菜单项
					SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 189, LWA_ALPHA | LWA_COLORKEY);
					POINT pt{};
					GetCursorPos(&pt);
					MoveWindow(hwnd, pt.x - 10, pt.y - 10, 20, 20, TRUE);
					return 0;
				}
				case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hwnd, &ps);

					{
						auto brush = CreateSolidBrush(RGB(0, 0, 0));
						auto pen = CreatePen(PS_NULL, 1, 0);  //创建空画笔
						auto old_brush = (HBRUSH)SelectObject(hdc, brush);
						auto old_pen = (HPEN)SelectObject(hdc, pen);
						Rectangle(hdc, 0, 0, 20, 20);
						SelectObject(hdc, old_brush);
						SelectObject(hdc, old_pen);
						DeleteObject(brush);
						DeleteObject(pen);
					}

					EndPaint(hwnd, &ps);
					return 0;
				}
				return 0;
				}

				return base_type::WindowProc(hwnd, uMsg, wParam, lParam);
			}
		};

		//简单检查
		for (const auto & one : items)
			utils_assert(one != u"未选择");

		auto & wnd = Window::create();
		ShowWindow(wnd.m_hwnd, SW_SHOW);
		UpdateWindow(wnd.m_hwnd);

		HMENU menu = CreatePopupMenu();
		utils_assert(menu);	//简单检查
		//AppendMenuW(menu, MF_SEPARATOR, NULL, NULL);//分隔条
		uint64_t id = 100;
		for (const auto & one : utils::iterator_and_give_me_i(items))
			AppendMenuW(menu, MF_STRING, id+one.i, (wchar_t*)one.value.c_str());

		POINT pt{};
		GetCursorPos(&pt);
		auto select = TrackPopupMenu(menu, TPM_RETURNCMD, pt.x, pt.y, 0, wnd.m_hwnd, NULL);
		DestroyMenu(menu);

		if (select == 0)
			return u"未选择";

		utils_assert(select >= id && select < items.size() + id); //简单检查
		return items[select - id];
	}
}