#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_2/Thread.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/MainWindow.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/Panel.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/ScrollPanel.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/Spring.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/StaticText.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/ImageWindow.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/CheckBox.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/StaticText.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/ScrollBar.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/Button.hpp>

//所有给用户用的类
namespace utils::windows::ui::window::window_system::summary
{
	using Thread = compile_time_dependency::layer_2::Thread;
	using WindowManager = compile_time_dependency::layer_2::WindowManager;
	using MainWindow = compile_time_dependency::layer_7::MainWindow;
	using Panel = compile_time_dependency::layer_7::Panel;
	using ScrollPanel = compile_time_dependency::layer_7::ScrollPanel;
	using VSpring = compile_time_dependency::layer_7::VSpring;
	using HSpring = compile_time_dependency::layer_7::HSpring;
	using StaticText = compile_time_dependency::layer_7::StaticText;
	using ImageWindow = compile_time_dependency::layer_7::ImageWindow;
	//using ScaleStyle = compile_time_dependency::layer_7::ScaleStyle;
}
namespace utils::windows::ui::window::window_system::summary::native
{
	using CheckBox = compile_time_dependency::layer_7::native::CheckBox;
	using StaticText = compile_time_dependency::layer_7::native::StaticText;
	using VScrollBar = compile_time_dependency::layer_7::native::ScrollBar<true>;
	using HScrollBar = compile_time_dependency::layer_7::native::ScrollBar<false>;
	using Button = compile_time_dependency::layer_7::native::Button;
}
namespace utils::windows::ui::window::window_system::summary::layout::leaf
{
	using LeafLayout = compile_time_dependency::layer_4::layout::method::leaf_layout::LeafLayout;
}
namespace utils::windows::ui::window::window_system::summary::layout::box
{
	using BoxLayout =      compile_time_dependency::layer_4::layout::method::box_layout::BoxLayout;
	using Setting =        compile_time_dependency::layer_4::layout::method::box_layout::Setting;
	using Align =          compile_time_dependency::layer_4::layout::method::box_layout::Align;
	using AlignValue =     compile_time_dependency::layer_4::layout::method::box_layout::AlignValue;
	using InsidePadding =  compile_time_dependency::layer_4::layout::method::box_layout::InsidePadding;
	using OutsidePadding = compile_time_dependency::layer_4::layout::method::box_layout::OutsidePadding;
}
namespace utils::windows::ui::window::window_system::summary::layout::scroll
{
	using ScrollLayout = compile_time_dependency::layer_4::layout::method::scroll_layout::ScrollLayout;
}

#if 0	//使用举例

void work()
{
	using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;

	namespace ws = utils::windows::ui::window::window_system::summary;

	struct s_ui
	{
		ws::Thread window_thread;
		std::unique_ptr<ws::WindowManager> window_manager;
		utils::pointer::not_null_for_user<ws::MainWindow> main_window;
	};

	s_ui ui;
	ui.window_thread.invoke_and_wait([&ui]()
	{
		ui.window_manager = std::make_unique<ws::WindowManager>();

		ui.main_window = &ui.window_manager->create_window<ws::MainWindow>();
		{
			auto layout_method = std::make_unique<ws::layout::box::BoxLayout>();
			auto _layout_method = layout_method.get();
			ui.main_window->m_layout_manager.set_layout_method(std::move(layout_method));
			_layout_method->set_layout_setting({
				.is_vertical_layout = true,
				.align = {
					.horizontal = ws::layout::box::LayoutAlignValue::Middle,
					.vertical = ws::layout::box::LayoutAlignValue::Middle,
				},
				.inside_padding = {
					.value = 20,
				},
				.outside_padding = {
					.left = 20,
					.right = 20,
					.top = 20,
					.bottom = 20,
				},
			});
		}
		ui.main_window->m_font_manager.m_font.set_value(
			utils::windows::ui::gdi::object::not_need_release::get_font(u8"微软雅黑", 50));
		ui.main_window->m_layout_manager._add_debug_note(u8"content", u8"主窗口");

		auto & panel = ui.main_window->m_window_manager.create_window<ws::Panel>();
		{
			auto layout_method = std::make_unique<ws::layout::box::BoxLayout>();
			auto _layout_method = layout_method.get();
			panel.m_layout_manager.set_layout_method(std::move(layout_method));
			_layout_method->set_layout_setting({
				.is_vertical_layout = true,
				.align = {
					.horizontal = ws::layout::box::LayoutAlignValue::Middle,
					.vertical = ws::layout::box::LayoutAlignValue::Middle,
				},
				.inside_padding = {
					.value = 20,
				},
				.outside_padding = {
					.left = 20,
					.right = 20,
					.top = 20,
					.bottom = 20,
				},
			});
		}
		panel.m_layout_manager._add_debug_note(u8"content", u8"菜板");

		auto & check_box_1 = panel.m_window_manager.create_window<ws::native::CheckBox>();
		check_box_1.m_text.set_value(u8"蛋糕1");
		check_box_1._debug_note.add_not_existed(u8"content", u8"蛋糕1");
		check_box_1.m_layout_manager._add_debug_note(u8"content", u8"蛋糕1");

		auto & check_box_2 = panel.m_window_manager.create_window<ws::native::CheckBox>();
		check_box_2.m_text.set_value(u8"蛋糕2");
		check_box_2._debug_note.add_not_existed(u8"content", u8"蛋糕2");
		check_box_2.m_layout_manager._add_debug_note(u8"content", u8"蛋糕2");
		
		auto & static_text_1 = panel.m_window_manager.create_window<ws::native::StaticText>();
		static_text_1.m_text.set_value(u8"牛奶1");
		static_text_1.m_text_color.set_value({180,180,180});
		static_text_1.m_text_background_color.set_value({0,0,0});
		static_text_1.m_text_align.set_value(ws::native::StaticText::TextAlignValue::Large);
		static_text_1._debug_note.add_not_existed(u8"content", u8"牛奶1");
		static_text_1.m_layout_manager._add_debug_note(u8"content", u8"牛奶1");

		s_hwnd(ui.main_window->get_hwnd()).set_visible(true);
	});

	for(double i = 0; ;i+=3.14/70)
	{
		Sleep(30);

		utils::u8 font_size = 5 + (int)((sin(i)+1)/2*70+0.5);

		ui.window_thread.invoke_and_wait([&ui, font_size]()
		{
			ui.main_window->m_font_manager.m_font.set_value(
				utils::windows::ui::gdi::object::not_need_release::get_font(u8"微软雅黑", font_size));
		});
	}

	while(1)
		Sleep(1);
}
int main()
{
	work();
	return 0;
}
#endif