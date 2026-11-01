#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	class MainWindow final : public layer_6::Window
	{
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
				on_message_create();
				break;
			case WM_CLOSE:
				return 0;
			}
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_OVERLAPPED;
		}
	private:
		void on_message_create()
		{
			get_s_hwnd().set_sysmenu_exist(true);
			get_s_hwnd().set_title_bar_exist(true);
			get_s_hwnd().set_max_button_exist(true);
			get_s_hwnd().set_min_button_exist(true);
			get_s_hwnd().set_thick_frame_exist(true);

			//这个版本，把整个窗口变为双缓冲的
			get_s_hwnd().set_double_buffer(true);
		}
	};
}