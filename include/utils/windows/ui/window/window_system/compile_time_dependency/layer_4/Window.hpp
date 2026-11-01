#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_3/Window.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_4/LayoutManager.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_4/TimerManager.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_4/FontManager.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4
{
	//在3和4层，完成 定时器,窗口布局,字体设置 的功能
	class Window : public layer_3::Window
	{
	public:
		LayoutManager m_layout_manager;
		TimerManager m_timer_manager;
		FontManager m_font_manager;
	public:
		Window()
			: m_layout_manager{ *this }
			, m_timer_manager{ *this }
			, m_font_manager{ *this }
		{
		}
	private:
		LRESULT wnd_proc_2(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch (uMsg)
			{
			case WM_SIZE:
			{
				//转发
				m_layout_manager.meet_window_message(hwnd, uMsg, wParam, lParam);

				//因为有的窗口要在改变大小时，拉伸其中内嵌的native控件，所以此处不直接返回
				break;
			}
			case WM_TIMER:
			{
				//转发
				return m_timer_manager.meet_window_message(hwnd, uMsg, wParam, lParam);
			}
			}

			//转发给更具体的窗口来处理
			return wnd_proc_3(hwnd, uMsg, wParam, lParam);
		}
		virtual LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	private:
		LayoutManager & get_layout_manager() override final
		{
			return m_layout_manager;
		}
	private:
		FontManager & get_font_manager() override final
		{
			return m_font_manager;
		}

		//添加常用函数
	public:
		// 设置字体
		void set_font(HFONT font)
		{
			//设置字体
			m_font_manager.m_font.set_value(font);
		}
		// 特化
		void set_font(const std::u8string & font_name, uint64_t font_size)
		{
			// 获取字体
			const auto font = utils::windows::ui::gdi::object::not_need_release::
				get_font(font_name, font_size);

			// 转发
			set_font(font);
		}
		//设置垂直布局
		void set_vbox_layout(const s8 inside_padding, const s8 outside_padding)
		{
			//转发
			return set_box_layout(true, inside_padding, outside_padding);
		}
		//设置垂直布局
		void set_hbox_layout(const s8 inside_padding, const s8 outside_padding)
		{
			//转发
			return set_box_layout(false, inside_padding, outside_padding);
		}
	private:
		//设置线性布局
		void set_box_layout(bool is_vertical_layout, const s8 inside_padding, const s8 outside_padding)
		{
			//布局的类型
			using BoxLayout = utils::ui::layout::layout_system::layout_method::box_layout::BoxLayout;

			//创建布局
			auto layout_method = std::make_unique<BoxLayout>();

			//复制指针
			auto _layout_method = layout_method.get();

			//设置布局
			m_layout_manager.set_layout_method(std::move(layout_method));

			//对齐的类型
			using AlignValue = utils::ui::layout::layout_system::layout_method::box_layout::AlignValue;

			//设置布局的选项
			_layout_method->set_all_setting({
				.is_vertical_layout = is_vertical_layout,
				.align = {
					.horizontal = AlignValue::Middle,
					.vertical = AlignValue::Middle,
				},
				.inside_padding = {
					.value = inside_padding,
				},
				.outside_padding = {
					.left = outside_padding,
					.right = outside_padding,
					.top = outside_padding,
					.bottom = outside_padding,
				},
			});
		}
	};
}