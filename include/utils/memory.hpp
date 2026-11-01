#pragma once

#include <memory>

namespace utils
{
	//对std::make_unique的简单包装
	template<typename T, typename ... T_args>
	void make_unique(std::unique_ptr<T> & value, T_args && ... args)
	{
		value = std::make_unique<T>(std::forward<T_args>(args)...);
	}
}
