#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_2/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4
{
	//向前声明
	class LayoutManager;
	class FontManager;
}
namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_3
{
	class Window : public layer_2::Window
	{
	private:
		friend class layer_4::LayoutManager;

		virtual layer_4::LayoutManager & get_layout_manager() = 0;

	private:
		friend class layer_4::FontManager;

		virtual layer_4::FontManager & get_font_manager() = 0;
	};
}