#pragma once

#include <functional>
#include <utils/default_value.hpp>

namespace utils
{
	namespace inner
	{
		template<typename T>
		class function : public std::function<T>
		{
		public:
			using std::function<T>::function;
			using std::function<T>::operator =;

		public:
			//如果函数存在则调用，否则返回默认值
			template<typename ... Args>
			typename std::function<T>::result_type call_if_exist(Args && ... args)
			{
				if (*this)
					return (*this)(std::forward<Args>(args)...);
				else
					return utils::default_value<typename std::function<T>::result_type>()();
			}
		};
	}
	template<typename T>
	auto function(const std::function<T> & f)
	{
		return (const inner::function<T> &)f;
	}
	template<typename T>
	auto function(std::function<T> & f)
	{
		return (inner::function<T> &)f;
	}
}