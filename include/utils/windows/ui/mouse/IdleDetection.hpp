#pragma once

#include <utils/time/steady_detection/noun/AutoUpdateValue.hpp>
#include <utils/windows/ui/mouse.hpp>

namespace utils::windows::ui::mouse
{
	//鼠标空闲检测
	class IdleDetection : public utils::time::steady_detection::noun::AutoUpdateValue<utils::Index2D>
	{
	private:
		//重写
		utils::Index2D get_current_value() const override final
		{
			//获得当前鼠标位置
			const auto current_value = utils::windows::ui::mouse::get_position();

			//给出
			return current_value;
		}
	};
}
