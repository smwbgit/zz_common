#pragma once

#include <atomic>
#include <vector>
#include <functional>

namespace utils::value
{
	//note: 线程安全:get() set()
	//note: 线程不安全:add_changed_listener()
	template<typename T>
	class EventedValue
	{
	private:
		std::atomic<T> m_value{};
		std::vector<std::function<void(const T & value)>> m_changed_listeners;
	public:
		T get() const
		{
			T value = m_value;
			return value;
		}
		void set(const T & value)
		{
			T old_value = m_value.exchange(value);
			bool is_same = old_value == value;

			if(!is_same)
				send_changed_event(value);
		}
		void add_changed_listener(std::function<void(const T & value)> listener)
		{
			m_changed_listeners.push_back(listener);
		}
	private:
		void send_changed_event(const T & value)
		{
			for(auto & listener : m_changed_listeners)
				listener(value);
		}
	};
}