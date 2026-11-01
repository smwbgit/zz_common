#pragma once

#include <functional>

namespace utils::algorithm
{
	//过滤: 只留下meet_one_value()返回true的
	template<typename T_container>
	void filter(T_container & all, std::function<bool(const typename T_container::value_type &)> meet_one_value)
	{
		T_container _all;
		for(const auto & one : all)
		{
			if(meet_one_value(one))
			{
				_all.push_back(std::move(one));
			}
		}
		all = std::move(_all);
	}
}