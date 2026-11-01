#pragma once

#include <chrono>
#include <utils/assert.hpp>

namespace utils::time
{
	//存储一个时间起始点
	class StartPosition
	{
	private:
		std::chrono::steady_clock::time_point m_start_position = std::chrono::steady_clock::now();
	public:
		//重新计时
		void reset()
		{
			m_start_position = std::chrono::steady_clock::now();
		}
		//获得已过去的时间
		std::chrono::steady_clock::duration get_passed_time() const
		{
			const auto now = std::chrono::steady_clock::now();
			return now - m_start_position;
		}
		//是否已过去足够的时间
		bool is_enough_passed(std::chrono::milliseconds time_length) const
		{
			const auto passed_time = get_passed_time();
			return passed_time >= time_length;
		}
	};
}