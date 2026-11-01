#pragma once

#include <vector>
#include <utils/iterator.hpp>
#include <utils/assert.hpp>

namespace utils
{
	//在数组中查找一个有且只有一个的元素的位置
	template<typename T>
	void find_existed_unique_entry(_Out_ uint64_t & pos, const T & value, const std::vector<T> & vec)
	{
		bool is_find{};
		for (const auto & e : utils::iterator_and_give_me_i(vec))
		{
			if (e.value == value)
			{
				utils_assert(is_find == false);
				is_find = true;
				pos = e.i;
			}
		}
		utils_assert(is_find);
	}

#pragma region 最小最大
	//获取最小值
	//e.g.: utils::min(3,4,5) == 3
	template<typename T>
	const T & (min)(const T & arg_0, const T & arg_1)
	{
		if (arg_0 < arg_1)
			return arg_0;
		else
			return arg_1;
	}
	//特化
	template<typename T, typename ... Args>
	const T & (min)(const T & arg_0, const T & arg_1, Args && ... args)
	{
		return utils::min<T>(utils::min<T>(arg_0, arg_1), std::forward<Args>(args)...);
	}

	//获取最大值
	//e.g.: utils::max(3,4,5) == 5
	template<typename T>
	const T & (max)(const T & arg_0, const T & arg_1)
	{
		if (arg_0 >= arg_1)
			return arg_0;
		else
			return arg_1;
	}
	//特化
	template<typename T, typename ... Args>
	const T & (max)(const T & arg_0, const T & arg_1, Args && ... args)
	{
		return utils::max<T>(utils::max<T>(arg_0, arg_1), std::forward<Args>(args)...);
	}
#pragma endregion
}