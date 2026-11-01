#pragma once

#include <functional>
#include <vector>

namespace utils::event
{
	//note: 非线程安全
	template<typename T>
	class Event
	{
	private:
		std::vector<std::function<T>> m_listeners;
	public:
		//添加监听
		void add_listener(std::function<T> listener)
		{
			m_listeners.push_back(listener);
		}
		//特化
		Event & operator += (std::function<T> listener)
		{
			//转发
			add_listener(listener);

			return *this;
		}
		//发出通知
		void notify() const
		{
			for(const auto & listener : m_listeners)
				listener();
		}
	};
}