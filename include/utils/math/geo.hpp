#pragma once

#include <math.h>
#include "utils/assert.hpp"

//几何相关
namespace utils::math::geo
{
	//π
	inline const double pi = acos(-1.0);

	//角度转弧度
	inline double to_radian(double degree)
	{
		return degree / 360 * 2 * pi;
	}
	//弧度转角度
	inline double to_degree(double radian)
	{
		return radian / (2 * pi) * 360;
	}
	//求sin
	inline double sin(double radian)
	{
		return ::sin(radian);
	}
	//求cos
	inline double cos(double radian)
	{
		return ::cos(radian);
	}
	//求acos
	inline double acos(double value)
	{
		//简单检查
		utils_assert(value <= 1 && value >= -1);

		return ::acos(value);
	}
	//求atan
	inline double atan(double value)
	{
		return ::atan(value);
	}
}