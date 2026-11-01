#pragma once

#include <utils/time/steady_detection/noun/AutoUpdateValue.hpp>
#include <utils/windows/ui/window/active_window.hpp>

namespace utils::windows::ui::window::active_window
{
	//窗口切换的稳定检测
	class SteadyDetection : public utils::time::steady_detection::noun::AutoUpdateValue<HWND>
	{
	private:
		//重写
		HWND get_current_value() const override final
		{
			//获得当前激活窗口
			const auto current_value = utils::windows::ui::window::active_window::get_active_window();

			//给出
			return current_value;
		}
	};
}