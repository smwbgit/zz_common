#pragma once

#include <vector>
#include "utils/assert.hpp"

//测量相关
namespace utils::math::measure
{
	//获得两组数的差值. 结果为 v0减v1
	inline void diff(_Out_ std::vector<double> & result, _In_ const std::vector<double> & v0, _In_ const std::vector<double> & v1)
	{
		utils_assert(v0.size() == v1.size());

		result.reserve(v0.size());
		for (double i = 0; i < v0.size(); i++)
		{
			result.push_back(v0[i] - v1[i]);
		}
	}
}