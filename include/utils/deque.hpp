#pragma once

#include <deque>
#include <utils/assert.hpp>

namespace utils
{
	namespace inner
	{
		template<typename T>
		class deque : public std::deque<T>
		{
			//新加的函数
		public:
			//返回最后一个元素
			T & last_one()
			{
				//简单检查
				utils_assert(!this->empty());

				return (*this)[this->size()-1];
			}
			//特化
			const T & last_one() const
			{
				//简单检查
				utils_assert(!this->empty());

				return (*this)[this->size()-1];
			}
		};
	}
	template<typename T>
	const inner::deque<T> & deque(const std::deque<T> & c)
	{
		return (const inner::deque<T> &)c;
	}
	template<typename T>
	inner::deque<T> & deque(std::deque<T> & c)
	{
		return (inner::deque<T> &)c;
	}
}