#pragma once

#include "utils/ui/Point.hpp"
#include "utils/ui/Color.hpp"

namespace utils::draw
{
	//绘画操作的抽象类
	class Drawer
	{
	public:
		//画直线: 从begin到end
		virtual void draw_line(utils::ui::Point begin, utils::ui::Point end) = 0;
		//画背景
		//color: 背景颜色
		virtual void fill_background(utils::ui::Color color) = 0;
		//画圆
		//pt: 圆心
		virtual void draw_circle(utils::ui::Point pt, double r) = 0;

	};
}