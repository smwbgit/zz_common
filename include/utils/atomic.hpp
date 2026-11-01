#pragma once

#include <mutex>
#include <functional>
#include <mutex>

namespace utils
{
	template<typename T>
	class atomic
	{
	private:
		std::mutex m_lock;
		T m_value{};
	public:
		template<typename T_return>
		T_return lock(std::function<T_return(T & value)> meet_locked)
		{
			std::lock_guard<std::mutex> guard{m_lock};
			return meet_locked(m_value);
		}
	};
}