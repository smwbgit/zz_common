#pragma once

#include "utils/computer_basic.hpp"

namespace utils
{
	//一个三维索引
	struct Index3D
	{
	public:
		int64_t i{};
		int64_t j{};
		int64_t k{};
				
		Index3D()
		{
		}
		template<typename T1, typename T2, typename T3>
		Index3D(T1 _i, T2 _j, T3 _k)
		{
			utils::computer_basic_function::store_this_integer(i, _i);
			utils::computer_basic_function::store_this_integer(j, _j);
			utils::computer_basic_function::store_this_integer(k, _k);
		}
		bool operator == (const Index3D & o) const
		{
			return
				i == o.i &&
				j == o.j &&
				k == o.k;
		}
		bool operator < (const Index3D & o) const
		{
			if (i != o.i)
				return i < o.i;
			else if (j != o.j)
				return j < o.j;
			else
				return k < o.k;
		}
	};
}