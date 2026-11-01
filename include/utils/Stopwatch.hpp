#pragma once

#include <chrono>
#include <functional>

namespace utils::stopwatch
{
	//计时器
	class Stopwatch
	{
	private:
		std::function<void(std::chrono::nanoseconds ns)> m_meet_finish;
		std::chrono::time_point<std::chrono::steady_clock> m_start = std::chrono::steady_clock::now();
		bool m_is_first{true};

	public:
		Stopwatch()
		{
		}
		Stopwatch(std::function<void(std::chrono::nanoseconds ns)> meet_finish)
			: m_meet_finish{meet_finish}
		{
		}
		~Stopwatch()
		{
			if (m_meet_finish)
				m_meet_finish(get_ns());
		}
		std::chrono::nanoseconds get_ns() const
		{
			auto end = std::chrono::steady_clock::now();
			auto ns = end - m_start;
			return ns;
		}
		//是否是第一次调用is_first这个函数
		bool is_first()
		{
			if (m_is_first)
			{
				m_is_first = false; return true;
			}
			return false;
		}
		//重新计时
		void restart()
		{
			m_start = std::chrono::steady_clock::now();
		}
		//获取总秒数
		double get_seconds() const
		{
			return (1.0 * get_ns().count() / std::nano::den);
		}
		//特化
		void get_seconds(_Out_ double& seconds) const
		{
			seconds = get_seconds();
		}
		//特化: 获取分钟数
		double get_minutes() const
		{
			return (1.0 * get_ns().count() / std::nano::den / 60);
		}
	};


	//特化: 专为循环中的，一组动作中的一个，而设计。举例如下:
#if 0
	while (1)
	{
		utils::stopwatch::LoopEntry stopwatch;

		...

		stopwatch.meet_actions();

		//一组动作
		if (...) { ...  continue; }
		if (...) { ...  continue; }
		if (stopwatch.is_long_enough(3s)) { ...  continue; }
		if (...) { ...  continue; }
		if (...) { ...  continue; }

		...
	}
#endif
	//上面的例子，会在第三个if持续经过，并保持了3s时，才执行
	class LoopEntry
	{
	private:
		Stopwatch m_stopwatch;
		bool m_flag{true};
	public:
		void meet_actions()
		{
			if (m_flag) m_stopwatch.restart();

			m_flag = true;
		}
		template<typename T>
		bool is_long_enough(const T & duration)
		{
			m_flag = false;
			auto _duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
			if (m_stopwatch.get_ns() >= _duration)
			{
				m_stopwatch.restart();
				return true;
			}
			return false;
		}
	};
}
namespace utils
{
	using utils::stopwatch::Stopwatch;
}


