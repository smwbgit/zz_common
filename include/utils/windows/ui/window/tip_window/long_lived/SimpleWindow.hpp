#pragma once

#include <utils/windows/ui/window/window_system/summary.hpp>
#include <utils/windows/ui/screen.hpp>

namespace utils::windows::ui::window::tip_window::long_lived
{
	class SimpleWindow
	{
	private:
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
		using WindowManager = utils::windows::ui::window::window_system::summary::WindowManager;
		using MainWindow = utils::windows::ui::window::window_system::summary::MainWindow;
		using StaticText = utils::windows::ui::window::window_system::summary::native::StaticText;
	private:
		utils::pointer::not_null_for_user<MainWindow> m_ui_main_window{};
		utils::pointer::not_null_for_user<StaticText> m_ui_static_text_1{};
	public:
		//note: 创建后的窗口，是不可见的，需要主动让它可见
		SimpleWindow(WindowManager & window_manager)
		{
			//创建主窗口
			m_ui_main_window = &window_manager.create_window<MainWindow>();

			//创建静态文本
			m_ui_static_text_1 = &m_ui_main_window->create_window<StaticText>();

			//设置主窗口的布局
			m_ui_main_window->set_vbox_layout(0, 0);

			//设置字体
			m_ui_main_window->set_font(u8"微软雅黑", 21);

			//设置主窗口的属性：把预设属性取消
			m_ui_main_window->get_s_hwnd().set_sysmenu_exist(false);
			m_ui_main_window->get_s_hwnd().set_title_bar_exist(false);
			m_ui_main_window->get_s_hwnd().set_max_button_exist(false);
			m_ui_main_window->get_s_hwnd().set_min_button_exist(false);
			m_ui_main_window->get_s_hwnd().set_thick_frame_exist(false);

			//设置主窗口的属性
			m_ui_main_window->get_s_hwnd().set_always_on_top(true);
			m_ui_main_window->get_s_hwnd().set_clip_children(true);
			m_ui_main_window->get_s_hwnd().set_visible_on_taskbar(false);
			m_ui_main_window->get_s_hwnd().set_is_watermark(true);
			m_ui_main_window->get_s_hwnd().set_watermark_alpha_and_color(220, RGB(0,0,0));

			//设置静态文本的属性
			m_ui_static_text_1->m_text_align.set_value(StaticText::TextAlignValue::Large);	//右对齐
			m_ui_static_text_1->m_text_color.set_value(utils::color::White);	//文本颜色
			m_ui_static_text_1->m_text_background_color.set_value(utils::color::Black);	//背景颜色

			//不断的把自己放在最前面, 保证即使和另一个HWND_TOPMOST窗口重叠，也能被看见
			keep_moving_to_topmost();
		}
		~SimpleWindow()
		{
			//销毁主窗口
			m_ui_main_window->m_window_manager.destroy_window();
		}
	public:
		//获取主窗口的s_hwnd
		s_hwnd get_s_hwnd()
		{
			//转发
			return m_ui_main_window->get_s_hwnd();
		}
		//设置可见性
		void set_visible(bool is_visible)
		{
			//设置主窗口的可见性
			m_ui_main_window->get_s_hwnd().set_visible(is_visible);
		}
		//设置文本颜色
		void set_text_color(utils::color::Color color)
		{
			//简单检查: 因为背景色是黑色，所以不能是黑色
			utils_assert(color != utils::color::Black);

			//设置文本颜色
			m_ui_static_text_1->m_text_color.set_value(color);
		}
		//设置文本
		void set_text(const std::u8string & text)
		{
			//设置文本
			m_ui_static_text_1->m_text.set_value(text);

			//获取主窗口的最优大小
			const auto & hint_size = m_ui_main_window->m_layout_manager.get_hint_size();

			//设置主窗口大小
			m_ui_main_window->get_s_hwnd().set_client_size(hint_size);

			//获取屏幕大小
			const auto screen_size = utils::windows::ui::screen::get_screen_size();

			//新的主窗口位置
			const utils::Index2D position{ screen_size.i - 73 - hint_size.i, 76 };

			//设置主窗口位置
			m_ui_main_window->get_s_hwnd().set_position(position);
		}
	private:
		//不断放在最前
		void keep_moving_to_topmost()
		{
			//添加一个定时器
			m_ui_main_window->m_timer_manager.add_timer(std::chrono::seconds(1), [this]()
			{
				//主窗口的句柄
				const auto hwnd = m_ui_main_window->get_hwnd();

				//把它放在最前面
				utils_assert(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
					SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE));
			});
		}
	};
}