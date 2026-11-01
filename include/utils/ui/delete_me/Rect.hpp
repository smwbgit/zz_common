#pragma once

#include "utils/ui/Point.hpp"
#include "utils/ui/Size.hpp"

namespace utils::ui
{
	//表示一个矩形
	class Rect
	{
	public:
		utils::ui::Point pt0;	//左上点
		utils::ui::Point pt1;	//右下点（不包括此点）

	public:
		Rect() = default;
		Rect(utils::ui::Point pt0, utils::ui::Point pt1)
			: pt0{pt0}
			, pt1{pt1}
		{
		}
		Rect operator + (const utils::ui::Point & pt) const
		{
			Rect rect = *this;
			rect.pt0 += pt;
			rect.pt1 += pt;
			return rect;
		}
		double width() const
		{
			return pt1.x - pt0.x;
		}
		double height() const
		{
			return pt1.y - pt0.y;
		}
	};
	Rect operator + (const utils::ui::Point& pt, const utils::ui::Rect & rect)
	{
		return rect + pt;
	}
}