#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/windows/window/ObjectBinder.hpp>
#include <utils/map.hpp>
#include <utils/character_encoding.hpp>

namespace utils::windows::window
{
#if 0
	//example:
	class Window : public utils::windows::window::TransparentWindow
	{
	private:
		using base_type = utils::windows::window::TransparentWindow;
	protected:	//已填写的设置项
		void get_LayeredWindowAttributes(COLORREF & crKey, BYTE & bAlpha, DWORD & dwFlags) override
		{
			//crKey = RGB(0, 0, 0);
			bAlpha = 128;
			dwFlags = LWA_ALPHA;// | LWA_COLORKEY;
		}
		LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
				case WM_CREATE:
				{
					auto size = utils::windows::monitor::get_screen_size();
					MoveWindow(hwnd, 0, 0, size.i, size.j, FALSE);
					break;
				}
				case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hwnd, &ps);

					RECT rect{};
					GetClientRect(hwnd, &rect);

					{
						auto brush = CreateSolidBrush(RGB(0, 0, 0));
						auto pen = CreatePen(PS_NULL, 1, 0);  //创建空画笔
						auto old_brush = (HBRUSH)SelectObject(hdc, brush);
						auto old_pen = (HPEN)SelectObject(hdc, pen);
						Rectangle(hdc, 0, 0, rect.right, rect.bottom);
						SelectObject(hdc, old_brush);
						SelectObject(hdc, old_pen);
						DeleteObject(brush);
						DeleteObject(pen);
					}

					EndPaint(hwnd, &ps);
					break;
				}
			}

			return base_type::WindowProc(hwnd, uMsg, wParam, lParam);
		}
	};
#endif
	class Window
	{
	protected:
		//一些常用的窗口类型
		enum class WindowStyle
		{
			透明窗口,
			具体指定,
		};
	protected:	//提供的设置项
		virtual void get_dwStyle(WindowStyle & window_style, DWORD & dwStyle, DWORD & dwExStyle) =0;
		virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	public:
		HWND m_hwnd{};

	public:
		template<typename T>
		static T & create()
		{

			auto class_name = get_class_name<T>();

			WNDCLASS wc{};
			//wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
			wc.cbWndExtra = sizeof(int*);
			wc.lpfnWndProc = utils::windows::window::ObjectBinder<T>::WindowProc;
			wc.hInstance = GetModuleHandleA(0);
			wc.lpszClassName = class_name.c_str();

			auto atom = RegisterClassW(&wc);

			WindowStyle window_style{}; DWORD dwStyle{}, dwExStyle{};
			((T*)0)->get_dwStyle(window_style, dwStyle, dwExStyle);
			switch (window_style)
			{
			case WindowStyle::透明窗口:
				dwStyle = WS_POPUP;
				dwExStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
				break;
			case WindowStyle::具体指定:
				break;
			default:
				utils_throw();
			}

			HWND hwnd = CreateWindowExW(
				dwExStyle,			// Optional window styles.
				class_name.c_str(),               // Window class   MAKEINTATOM(atom)
				L"",		// Window text
				dwStyle,            // Window style

				// Size and position
				0, 0, 0, 0,

				NULL,       // Parent window    
				NULL,       // Menu
				GetModuleHandleA(0),  // Instance handle
				NULL        // Additional application data
			);

			utils_assert(hwnd);

			auto & wnd = utils::windows::window::ObjectBinder<T>::get_object(hwnd);
			wnd.m_hwnd = hwnd;
			return wnd;
		}
	private:
		template<typename T>
		friend class utils::windows::window::ObjectBinder;
	private:
		template<typename T>
		static std::wstring get_class_name()
		{
			static utils::map<std::string, uint64_t> table;
			table.insert_if_not_exist(typeid(T).name(), table.size());

			return utils::character_encoding::convert_cp936_to_unicode(
				"utils::windows::window::Window " +
				std::to_string(table.get_existed_value(typeid(T).name())));
		}
	};

	//透明窗口
	class TransparentWindow : public utils::windows::window::Window
	{
	private:
		using base_type = utils::windows::window::Window;
	protected:	//提供的设置项
		virtual void get_LayeredWindowAttributes(COLORREF & crKey, BYTE & bAlpha, DWORD & dwFlags) = 0;
		LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_CREATE:
			{
				COLORREF crKey = RGB(0, 0, 0);
				BYTE bAlpha = 128;
				DWORD dwFlags = LWA_ALPHA | LWA_COLORKEY;
				get_LayeredWindowAttributes(crKey, bAlpha, dwFlags);
				SetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
				break;
			}
			}

			return base_type::WindowProc(hwnd, uMsg, wParam, lParam);
		}
	protected:	//已填写的设置项
		void get_dwStyle(WindowStyle & window_style, DWORD & dwStyle, DWORD & dwExStyle) override
		{
			window_style = WindowStyle::透明窗口;
		}
	};

}