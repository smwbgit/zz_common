#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_5/Window.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/AnimationManager.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_6
{
	//在5层和6层，完成 动画 的功能
	class Window : public layer_5::Window
	{
	public:
		AnimationManager m_animation_manager;
	public:
		Window()
			: m_animation_manager{ *this }
		{
		}
	private:
		AnimationManager & get_animation_manager() override final
		{
			return m_animation_manager;
		}
	};
}