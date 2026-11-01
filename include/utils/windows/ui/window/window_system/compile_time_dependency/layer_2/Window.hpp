#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_1/Window.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_2/WindowManager.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_2
{
	//在1,2层，完成 窗口的创建/保存/销毁 的功能
	class Window : public layer_1::Window
	{
	public:
		WindowManager m_window_manager;
	public:
		Window()
			: m_window_manager{ *this }
		{
		}
	private:
		friend class layer_2::WindowManager;

		auto get_window_manager() -> WindowManager &  override final
		{
			return m_window_manager;
		}
	private:
		LRESULT wnd_proc_1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch (uMsg)
			{
			case WM_NCCREATE:
			case WM_DESTROY:
			case WM_NCDESTROY:
				utils_throw();

			case WM_CREATE:
				on_message_create();
				break;
			}

			//转发给更具体的窗口来处理
			return wnd_proc_2(hwnd, uMsg, wParam, lParam);
		}
		virtual LRESULT wnd_proc_2(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	private:
		void on_message_create()
		{
			s_hwnd(get_hwnd()).set_clip_children(true);
			s_hwnd(get_hwnd()).set_clip_siblings(true);
		}
	public:
		//创建窗口
		//note: 比较常用，在这里创建一个转发函数，方便使用
		template<typename window_T>
		window_T & create_window()
		{
			//转发
			return m_window_manager.create_window<window_T>();
		}
	};
}