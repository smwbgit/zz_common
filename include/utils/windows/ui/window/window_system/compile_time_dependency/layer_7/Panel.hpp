#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	class Panel final : public layer_6::Window
	{
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_CHILD | WS_VISIBLE;
		}
	};
}