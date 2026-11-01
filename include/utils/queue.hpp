#pragma once

#include <queue>
#include <functional>

namespace utils
{
	namespace inner
	{
		template<typename T>
		class queue : public std::queue<T>
		{
		public:
			//放入一些值
			template<typename T_container>
			void push_many(const T_container & values)
			{
				for(const auto & one : values)
				{
					this->push(one);
				}
			}
			//弹出所有, 每弹出一个,调用一次回调函数
			void pop_all(std::function<void(T &&)> meet_one_value)
			{
				while (1)
				{
					if (this->empty()) return;

					auto value = std::move(this->front());
					this->pop();

					//给出
					meet_one_value(std::move(value));
				}
			}
			[[nodiscard]] T pop()
			{
				const auto value = this->front(); std::queue<T>::pop();
				return value;
			}
		};
	}
	template<typename T>
	const inner::queue<T> & queue(const std::queue<T> & c)
	{
		return (const inner::queue<T> &)c;
	}
	template<typename T>
	inner::queue<T> & queue(std::queue<T> & c)
	{
		return (inner::queue<T> &)c;
	}
}