#pragma once

#include <utils/ui/layout/layout_system/common.hpp>

namespace utils::ui::layout::layout_system::layout_method::box_layout
{
	//外边距: 窗口边缘与子窗口之间
	using OutsidePadding = utils::ui::layout::OutsidePadding;
	//内边距: 子窗口之间
	struct InsidePadding
	{
		s8 value{};
	};
	//对齐方式
	using AlignValue = utils::ui::layout::AlignValue;
	//对齐方式
	using Align = utils::ui::layout::Align;
	//布局设置
	struct Setting
	{
		bool is_vertical_layout{};	//布局方向: true 为垂直布局, false 为水平布局
		Align align;	//对齐  当有空余空间分配不出去时，使用此值
		InsidePadding inside_padding;
		OutsidePadding outside_padding;
	};
}