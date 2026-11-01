#pragma once

#include <utils/windows/ui/window/window_system/summary.hpp>
#include <utils/windows/ui/screen.hpp>

namespace utils::windows::ui::window::tip_window::short_lived
{
	namespace inner
	{
		// 提示窗口
		class TipWindow
		{
		private:
			using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
			using Thread = utils::windows::ui::window::window_system::summary::Thread;
			using WindowManager = utils::windows::ui::window::window_system::summary::WindowManager;
			using MainWindow = utils::windows::ui::window::window_system::summary::MainWindow;
			using StaticText = utils::windows::ui::window::window_system::summary::StaticText;
		private:
			static inline const std::chrono::milliseconds m_exist_time{ 2600 };
			static inline const u8 m_alpha_begin{ 220 };
			std::chrono::steady_clock::time_point m_create_time;
			utils::windows::ui::gdi::object::font::Font m_font;
			utils::pointer::not_null_for_user<MainWindow> m_ui_main_window{};
			utils::pointer::not_null_for_user<StaticText> m_ui_static_text_1{};
			bool m_is_finished{ false };
		public:
			TipWindow(WindowManager & window_manager, const std::u8string & text, utils::color::Color color)
			{
				//创建主窗口
				m_ui_main_window = &window_manager.create_window<MainWindow>();

				//创建静态文本
				m_ui_static_text_1 = &m_ui_main_window->create_window<StaticText>();

				//设置主窗口的布局
				m_ui_main_window->set_vbox_layout(0, 0);

				//创建字体
				m_font = utils::windows::ui::gdi::object::font::create_font([](auto & param)
				{
					param.name = u8"微软雅黑";
					param.size = 70;
					param.weight = 560;
					param.quality = NONANTIALIASED_QUALITY;
				});

				//设置字体
				m_ui_main_window->set_font(m_font);

				//设置主窗口的属性：把预设属性取消
				m_ui_main_window->get_s_hwnd().set_sysmenu_exist(false);
				m_ui_main_window->get_s_hwnd().set_title_bar_exist(false);
				m_ui_main_window->get_s_hwnd().set_max_button_exist(false);
				m_ui_main_window->get_s_hwnd().set_min_button_exist(false);
				m_ui_main_window->get_s_hwnd().set_thick_frame_exist(false);

				//设置主窗口的属性
				m_ui_main_window->get_s_hwnd().set_clip_children(true);
				m_ui_main_window->get_s_hwnd().set_visible_on_taskbar(false);
				m_ui_main_window->get_s_hwnd().set_always_on_top(true);
				m_ui_main_window->get_s_hwnd().set_is_watermark(true);
				m_ui_main_window->get_s_hwnd().set_watermark_alpha_and_color(m_alpha_begin, utils::color::Black);

				//设置静态文本的属性
				m_ui_static_text_1->m_align.set_value({StaticText::AlignValue::Large, StaticText::AlignValue::Small});	//水平方向上,右对齐.垂直方向上,上对齐
				m_ui_static_text_1->m_text_color.set_value(utils::color::White);	//文本颜色
				m_ui_static_text_1->m_background_color.set_value(utils::color::Black);	//背景颜色
				set_text(text);
				set_text_color(color);

				//记录当前时间
				m_create_time = std::chrono::steady_clock::now();

				//添加定时器: 销毁
				m_ui_main_window->m_timer_manager.add_timer(m_exist_time, [this]()
				{
					//销毁主窗口
					m_ui_main_window->m_window_manager.destroy_window();

					//更新状态
					m_is_finished = true;
				});

				//添加定时器: 淡出
				m_ui_main_window->m_timer_manager.add_timer(std::chrono::milliseconds{ 1800 }, [this]()
				{
					//简单检查
					utils_assert(m_exist_time > std::chrono::milliseconds{1800});

					//开始淡出
					m_ui_main_window->m_animation_manager.disappear_by_alpha(m_exist_time - std::chrono::milliseconds{1800});
				});

				//让主窗口可见
				m_ui_main_window->get_s_hwnd().set_visible(true);
			}
		public:
			//获取主窗口的s_hwnd
			s_hwnd get_s_hwnd()
			{
				//转发
				return m_ui_main_window->get_s_hwnd();
			}
			//获取主窗口
			MainWindow & get_main_window()
			{
				//给出
				return *m_ui_main_window;
			}
			//是否已经显示完毕
			bool is_finished() const
			{
				return m_is_finished;
			}
		private:
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
				const utils::Index2D position{ screen_size.i - 73 - hint_size.i, 72 };

				//设置主窗口位置
				m_ui_main_window->get_s_hwnd().set_position(position);
			}
		};

		//负责把提示窗口,适时的往下放放
		class TipWindowManager
		{
			//简便写法
		private:
			using Thread = utils::windows::ui::window::window_system::summary::Thread;
			using WindowManager = utils::windows::ui::window::window_system::summary::WindowManager;
		private:
			std::unique_ptr<WindowManager> m_window_manager;
			std::unique_ptr<Thread> m_thread;
			std::vector<std::unique_ptr<TipWindow>> m_all_tip_window;
		public:
			TipWindowManager()
			{
				//所有提示窗口,共用一个线程和一个窗口管理器

				//创建新线程
				m_thread = std::make_unique<Thread>();

				//创建窗口管理器
				m_thread->invoke_and_wait([this]()
				{
					m_window_manager = std::make_unique<WindowManager>();
				});
			}
			//显示提示
			void show(const std::u8string & text, utils::color::Color color)
			{
				//在窗口线程
				m_thread->invoke_and_wait([&text, &color, this]()
				{
					//创建窗口
					auto window = std::make_unique<TipWindow>(*m_window_manager, text, color);

					//清理已经不在的窗口
					utils::vector(m_all_tip_window).remove_if([](const auto & one)
					{
						return one->is_finished();
					});

					//把现有的,往下挪一挪
					for(const auto & one : m_all_tip_window)
					{
						//for debug
						namespace position_adjust = utils::windows::ui::window::window_system::compile_time_dependency::layer_6::position_adjust;

						//挪动一个
						one->get_main_window().m_animation_manager.
							move_to_by_offset(std::chrono::seconds{1},{0, 92},position_adjust::匀减速运动(-1));
					}

					//保存新窗口
					m_all_tip_window.push_back(std::move(window));
				});
			}
		};
	}

	//在屏幕右上角短暂显示一条提示
	inline void show(const std::u8string & text, utils::color::Color color = utils::color::Green)
	{
		//创建管理器
		static auto tip_window_manager = std::make_unique<inner::TipWindowManager>();

		//转发
		tip_window_manager->show(text, color);
	}
}