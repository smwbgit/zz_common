#pragma once

#include <utils/Index2D.hpp>

namespace utils::math::overlap::full_overlap
{
	//获得允许的范围
	//note: 给定两个线段，算出在完全覆盖的情况下，允许的可移动范围
	//window_size: 父窗口大小
	//child_window_size: 子窗口大小
	//return: 子窗口的左边的最远位置
	//        如果 父窗口大，子窗口小，此值为正
	//        如果 父窗口小，子窗口大，此值为负
	inline s8 get_allow_range(const s8 window_size, const s8 child_window_size)
	{
		//简单检查
		utils_assert(window_size > 0);
		utils_assert(child_window_size > 0);

		return window_size - child_window_size;
	}
	//特化
	//note: 给定两个矩形，算出在完全覆盖的情况下，允许的可移动范围
	//window_size: 父窗口大小
	//child_window_size: 子窗口大小
	//return: 子窗口的左上角的最远位置
	//        如果 父窗口大，子窗口小，此值为正
	//        如果 父窗口小，子窗口大，此值为负
	inline utils::Index2D get_allow_range(utils::Index2D window_size, utils::Index2D child_window_size)
	{
		//分为两个方向，转发
		return {
			get_allow_range(window_size.i, child_window_size.i),
			get_allow_range(window_size.j, child_window_size.j),
		};
	}
}