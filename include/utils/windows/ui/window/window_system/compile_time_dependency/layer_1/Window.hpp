#pragma once

#include <windows.h>
#include <utils/windows/ui/window/s_hwnd.hpp>
#include <utils/debug/DebugNote.hpp>
#include <utils/ui/window/visit_manager.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_2
{
	//前向声明
	class WindowManager;
}
namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_1
{
	class Window
		: public utils::ui::window::visit_manager::modifier::Visitable	//可被 访问管理器 遍历
	{
	private:
		std::optional<HWND> m_hwnd;
	public:
		Window() = default;
		virtual ~Window() = 0;
	public:
		//获取窗口句柄
		HWND get_hwnd() const
		{
			//简单检查
			utils_assert(m_hwnd);
			utils_assert(IsWindow(*m_hwnd));

			return *m_hwnd;
		}
	public:
		//向下转换为更具体的类型
		template<typename T>
		T & _down_cast()
		{
			//简单检查
			static_assert(std::is_base_of_v<Window, T>);

			T* p = dynamic_cast<T*>(this);

			//简单检查
			utils_assert(p);

			return *p;
		}
	private:
		friend class layer_2::WindowManager;
		
		//设置窗口句柄
		void set_hwnd(HWND hwnd)
		{
			//简单检查
			utils_assert(!m_hwnd);

			//保存
			m_hwnd = hwnd;
		}
		//窗口过程函数
		//note: 关于窗口的创建和销毁，已经由上层的 WindowManager::wnd_proc 处理
		//      此处处理其他消息即可
		virtual LRESULT wnd_proc_1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
		//获取窗口管理器
		virtual auto get_window_manager() -> layer_2::WindowManager &  = 0;
	protected:
		//便利声明
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
	public:
		//便利函数
		s_hwnd get_s_hwnd() const
		{
			return { get_hwnd() };
		}
	public:
		utils::debug::DebugNote _debug_note;
	};
	inline Window::~Window() = default;
}