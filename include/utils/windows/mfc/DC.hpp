#pragma once

#include "utils/draw/Drawer.hpp"
#include "utils/assert.hpp"

namespace utils::mfc
{
	class DC : public ::CDC, public utils::draw::Drawer
	{
	public:
		//实现
		void draw_line(utils::ui::Point begin, utils::ui::Point end) override
		{
			MoveTo((int)begin.x, (int)begin.y);
			LineTo((int)end.x, (int)end.y);
		}
		//实现
		void fill_background(utils::ui::Color color) override
		{
			CRect rect{};
			utils_assert(SIMPLEREGION == GetClipBox(&rect));	//todo 看看这里需不需要改进

			FillSolidRect(&rect, RGB(color.r, color.g, color.b));
		}
		//实现
		void draw_circle(utils::ui::Point pt, double r) override
		{
			Ellipse(CRect(pt.x - r, pt.y - r, pt.x + r, pt.y + r));
		}
	};
}

