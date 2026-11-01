#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <utils/assert.hpp>

namespace utils::thread
{
	//一个简单的标志类，用于线程间同步
	//note: 只能激发一次, 之后调用的wait()都不会再等待
	class Flag
	{
	private:
		mutable std::mutex m_mutex;
		mutable std::condition_variable m_cv;
		std::atomic<bool> m_value{false};
	public:
		bool is_true() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_value;
		}
		void set_true()
		{
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				utils_assert(false == m_value);
				m_value = true;
			}
			m_cv.notify_all();
		}
		void wait() const
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cv.wait(lock, [this]() ->bool { return m_value; });
		}
		operator bool() const
		{
			return is_true();
		}
	};
}