#pragma once

#include <functional>
#include <vector>
#include "utils/assert.hpp"
#include "utils/iterator.hpp"

//求解方法: 渐进法求方程
namespace utils::math::method::successive_approximation
{
	/*
	从一点向四周求解
	is_better: 传入两个点，返回第一点是否优于第二点
	pt: 起始位置。返回时，将包含求解后的值
	steps: 每一次试探的精度

	example:
		auto f = [](const std::vector<double> & pt)
		{
			double a = f1(pt[0], pt[1]);
			double b = f2(pt[0], pt[1]);
			return sqrt(a * a + b * b);
		};
		auto is_better = [f](const std::vector<double> & pt0, const std::vector<double> & pt1)
		{
			auto v0 = f(pt0);
			auto v1 = f(pt1);
			return v0 < v1;
		};
		std::vector<double> pt = {to_radius(30), 1.7};
		utils::math::method::successive_approximation::from_one_point(is_better, pt, {0.01, 0.1});
		utils::math::method::successive_approximation::from_one_point(is_better, pt, {0.0001, 0.001});
		utils::math::method::successive_approximation::from_one_point(is_better, pt, {0.000001, 0.00001});
	*/
	inline void from_one_point(_In_ std::function<bool(std::vector<double> pt0, std::vector<double> pt1)> /*pt0*/is_better/*than pt1*/, _Inout_ std::vector<double> & pt, _In_ const std::vector<double> & steps)
	{
		//简单检查
		utils_assert(pt.size() >= 1);
		utils_assert(pt.size() == steps.size());

		//准备函数
		/*
			移动一个分量，直到最优
			返回: 此分量是否发生了移动
		*/
		auto move_component = [is_better, steps](uint64_t i, _Inout_ std::vector<double> & pt)
		{
			//准备函数
			/*
				增加一个分量的值
				dir: 增加还是减小   0增加  1减小
			*/
			auto inc_component = [steps](uint64_t i, _Inout_ std::vector<double> & pt , uint64_t dir)
			{
				//简单检查
				utils_assert(i < pt.size());

				if (dir == 0)
					pt[i] += steps[i];
				else if (dir == 1)
					pt[i] -= steps[i];
				else
					utils_throw();
			};

			//保存原始值
			const auto old_pt = pt;

			//向左查找一次，向右查找一次
			for (uint64_t dir = 0; dir < 2; dir++)
			{
				while (true)
				{
					//下一点
					auto pt1 = pt;
					inc_component(i, pt1, dir);

					//如果原来的更好，则不再移动
					if (is_better(pt, pt1))
						break;

					//更新状态
					pt = pt1;
				}
			}

			//给出是否有变化
			return pt != old_pt;
		};

		//不停循环，直到所有分量都不再变化
		while(1)
		{
			bool is_changed = false;

			for (const auto i : utils::iterator_index(pt))
			{
//				is_changed || = move_component(i, pt);
				is_changed = is_changed || move_component(i, pt);
			}

			if (is_changed == false)
				break;
		}
	}

	/*
	从一点到另一点
	is_ok: 传入x，返回x是否已到达想找的区域
	x_begin, x_end: x的起始和终止位置
	steps: 每一次试探的精度

	example:
		auto func = [=](double x)
		{
			return x*x + sqrt(x) + sin(x);
		};

		auto func2 = [=](double x)
		{
			return func(x) < 8;
		};
		double x = utils::math::method::successive_approximation::from_begin_to_end(func2, 0, 180, { 0.1, 0.0001, 0.0000001 });
	*/
	inline double from_begin_to_end(std::function<bool(double x)> is_ok, double x_begin, double x_end, const std::vector<double>& steps)
	{
		double result{};	//每次求出来，保存一下，方便最后给出
		double begin{ x_begin };	//下次迭代时的开始位置

		for (const auto step : steps)
		{
			//做下标记，断言每次都能找到一个值
			bool is_find = false;

			for (double x = begin; x < x_end; x += step)
			{
				if (is_ok(x))
				{
					result = x;

					//退回一格，更精确查找
					begin = x - step;
					if (begin < x_begin)	//如果退一格会超出给的区域，就设为区域起始位置
						begin = x_begin;

					//做下标记
					is_find = true;
					break;
				}
			}

			//简单检查
			utils_assert(is_find);
		}
		return result;
	}
}