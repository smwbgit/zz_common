#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	namespace concepts
	{
		template<typename T>
		concept ScrollBar = utils::ui::layout::layout_system::layout_method::scroll_layout::single_orientation::concepts::ScrollBar<T>;
	}

	//滚动视图
	//note: 这个版本，先不从Panel继承 
	class ScrollPanel final : public layer_6::Window
	{
	private:
		using ScrollLayout = utils::ui::layout::layout_system::layout_method::scroll_layout::ScrollLayout;
	private:
		utils::pointer::not_null_for_user<ScrollLayout> m_scroll_layout;
	public:
		//添加滚动条
		void add_scroll_bar(bool is_vertical, concepts::ScrollBar auto & scroll_bar)
		{
			//相应的方向分量
			auto & single_orientation = is_vertical ?
				m_scroll_layout->m_vertical :
				m_scroll_layout->m_horizontal;

			//添加滚动条
			single_orientation.add_scroll_bar(scroll_bar);
		}
	private:
		//设置为滚动布局
		void set_scroll_layout()
		{
			//创建布局
			auto scroll_layout = std::make_unique<ScrollLayout>();

			//保存指针副本
			m_scroll_layout = scroll_layout.get();

			//设置布局
			m_layout_manager.set_layout_method(std::move(scroll_layout));
		}
		void on_message_create()
		{
			//设置为滚动布局
			set_scroll_layout();
		}
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
				on_message_create();
				break;
			}
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