#pragma once

// 此函数即将被删除，请改为使用 utils::windows::ui::window::tip_window::short_lived::show()

#include <chrono>
#include <thread>
#include <windows.h>
#include <utils/assert.hpp>
#include <utils/windows/ui/window/create.hpp>
#include <utils/windows/ui/window/paint.hpp>

namespace utils::windows::ui::tip_window
{
	namespace inner::show
	{
		class TipWindow
		{
		private:
			static const int exist_time = 2600;
			std::chrono::steady_clock::time_point create_time;
			std::u8string m_old_text;
			std::u8string m_text;
			COLORREF m_color{};

		public:
			void set_args(const std::u8string & text, COLORREF color)
			{
				m_text = text;
				m_color = color;
			}
			LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				switch (uMsg)
				{
				case WM_CREATE:
					return 0;
				case utils::windows::ui::window::create::user_message::UM_CREATE:
					SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 189, LWA_ALPHA | LWA_COLORKEY);
					SetTimer(hwnd, 1, exist_time, 0);
					SetTimer(hwnd, 2, 1800, 0);
					create_time = std::chrono::steady_clock::now();
					return 0;
				case WM_TIMER:
					switch (wParam)
					{
					case 1:
						DestroyWindow(hwnd);
						return 0;
					case 2:
						//开始淡出
						SetTimer(hwnd, 3, 10, 0);
						return 0;
					case 3:
						auto 存在时间 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - create_time).count();
						auto 进度 = 1.*(存在时间-1700)/ (exist_time - 1700);
						auto alpha = (1 - 进度) * 189;
						SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), (int)alpha, LWA_ALPHA | LWA_COLORKEY);
						return 0;
					}
				case WM_SIZE:
					break;
				case WM_PAINT:
				{
					utils::windows::ui::window::paint::begin_paint_with_memdc(hwnd, [this, hwnd](HDC memdc)
					{
						RECT rect{};
						utils_assert(GetWindowRect(hwnd, &rect));

						{
							auto brush = CreateSolidBrush(RGB(0, 0, 0));
							auto pen = CreatePen(PS_NULL, 1, 0);  //创建空画笔
							auto old_brush = (HBRUSH)SelectObject(memdc, brush);
							auto old_pen = (HPEN)SelectObject(memdc, pen);
							Rectangle(memdc, 0, 0, rect.right, rect.bottom);
							SelectObject(memdc, old_brush);
							SelectObject(memdc, old_pen);
							DeleteObject(brush);
							DeleteObject(pen);
						}
						{
							auto font = CreateFontW(90, 0, 0, 0, 560, 0, 0, 0, DEFAULT_CHARSET,
								OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FF_DONTCARE,
								L"微软雅黑");	utils_assert(font);
							auto old_font = SelectObject(memdc, font);

							SetBkMode(memdc, TRANSPARENT);
							SetTextColor(memdc, m_color);

							auto text_u16 = utils::text::encoding::unicode::utf8_to_utf16(m_text);
							if (m_old_text != m_text)
							{
								m_old_text = m_text;

								SIZE size{};
								utils_assert(GetTextExtentPoint32W(memdc, (wchar_t*)text_u16.c_str(),
									(int)text_u16.length(), &size));

								MoveWindow(hwnd, 38, 30, size.cx, size.cy, TRUE);
							}
							else
							{
								TextOutW(memdc, 0, 0, (wchar_t*)text_u16.c_str(), (int)text_u16.length());
							}

							SelectObject(memdc, old_font);
							DeleteObject(font);
						}
					});
					return 0;
				}
				case WM_CLOSE:
					break;
				case WM_DESTROY:
					PostQuitMessage(0);		//退出当前线程
					break;
				case WM_NCDESTROY:
					break;
				}

				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		};
	}
	//在屏幕右上角短暂显示一条提示
	inline void show(const std::u8string & text, COLORREF color = RGB(0, 255, 0))
	{
		utils::windows::ui::window::create::in_new_thread<inner::show::TipWindow>(
			u8"utils::windows::tip_window::show",
			WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			WS_POPUP, 0, 0, 100, 100, u8"",
			text, color
		);
	}
}