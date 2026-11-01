#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_4/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_6
{
	//向前声明
	class AnimationManager;
}
namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_5
{
	class Window : public layer_4::Window
	{
	private:
		friend class layer_6::AnimationManager;

		virtual layer_6::AnimationManager & get_animation_manager() = 0;
	};
}