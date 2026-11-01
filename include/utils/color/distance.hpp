#pragma once

#include <utils/color/space/lab.hpp>

//颜色距离
namespace utils::color::distance
{
	//获取颜色距离
	inline double get(utils::color::Color color_1, utils::color::Color color_2)
	{
		//把颜色1，从rgb颜色空间转为lab颜色空间
		const auto lab_1 = utils::color::space::lab::from_rgb(color_1);

		//把颜色2，从rgb颜色空间转为lab颜色空间
		const auto lab_2 = utils::color::space::lab::from_rgb(color_2);

		//获取两种颜色的距离
		const auto distance = utils::color::space::lab::get_color_distance(lab_1, lab_2);

		//给出
		return distance;
	}
}