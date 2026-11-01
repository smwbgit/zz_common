#pragma once

#include <math.h>
#include "utils/assert.hpp"

//代数相关
namespace utils::math::algebra
{

#pragma region 单个数的运算
	//开根号
	inline double sqrt(double value)
	{
		//简单检查
		utils_assert(value >= 0);

		return ::sqrt(value);
	}
#pragma endregion

#pragma region 插值
	//插值
	//note: 从区间[begin_0, end_0]里的value_0，映射到[begin_1, end_1]里的某个值
	inline double interpolate(
		_In_ const double begin_1, _In_ const double end_1,
		_In_ const double begin_0, _In_ const double end_0, _In_ const double value_0)
	{
		return (value_0 - begin_0) * (end_1 - begin_1) / (end_0 - begin_0) + begin_1;
	}
	//特化：中文名称
	inline double 插值(
		_In_ const double begin_1, _In_ const double end_1,
		_In_ const double begin_0, _In_ const double end_0, _In_ const double value_0)
	{
		return interpolate(begin_1, end_1, begin_0, end_0, value_0);
	}
#pragma endregion

#pragma region 多个数的运算
	//把数组的长度变为指定长度
	//example:
	//  [1,3,5] -> [1,2,3,4,5]
	//  [1,2,3,4,5] -> [1,3,5]
	inline void scale_array_size(_Out_ std::vector<double> & _values, _In_ const std::vector<double> & values, _In_ const double new_size)
	{
		if (values.empty())
		{
			_values.clear();
			return;
		}

		if (new_size <= 0)
		{
			_values.clear();
			return;
		}

		if (new_size == values.size())
		{
			_values = values;
			return;
		}

		//算出插值位置，在values数组中的哪两个数中间。然后按小数部分插值

		_values.resize(values.size());
		for (double i = 1; i < _values.size() - 1; i++)	//计算新的值：除了第一个和最后一个
		{
			double pos = utils::math::algebra::插值(0, values.size() - 1, 0, _values.size() - 1, i);
			double 整数部分 = floor(pos);
			double 小数部分 = pos - 整数部分;

			double v0 = values[整数部分];
			double v1 = values[整数部分 + 1];
			_values[i] = utils::math::algebra::插值(v0, v1, 0, 1, 小数部分);
		}
		_values[0] = values[0];	//第一个的值
		_values[_values.size() - 1] = values[values.size() - 1];	//最后一个的值
	}
#pragma endregion
}