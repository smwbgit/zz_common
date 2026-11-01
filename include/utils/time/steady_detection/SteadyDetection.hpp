#pragma once

#include <chrono>
#include <utils/assert.hpp>

namespace utils::time::steady_detection
{
	//稳定检测
	template<typename value_T>
	class SteadyDetection
	{
		//note: 为了能让用它的类，不必加mutable，所以在这里，把所有的成员加上。
	private:
		mutable std::chrono::steady_clock::time_point m_last_time{};
		mutable std::optional<value_T> m_last_value;
	public:
		//更新当前值
		void update_value(const value_T & value) const
		{
			if(!m_last_value.has_value())
			{
				m_last_value = value;
				m_last_time = std::chrono::steady_clock::now();
				return;
			}
			if(value != m_last_value.value())
			{
				m_last_value = value;
				m_last_time = std::chrono::steady_clock::now();
			}
		}
		//是否足够稳定
		//time_length: 到达稳定状态，需要的时间
		//rest_time: 到达稳定状态，还需要的时间
		template<typename rest_time_T>
			requires requires {
				std::chrono::duration_cast<rest_time_T>(std::declval<std::chrono::milliseconds>());
			}
		bool is_enough_steady(std::chrono::milliseconds time_length, rest_time_T & rest_time) const
		{
			//简单检查
			utils_assert(m_last_value.has_value());

			//当前时间
			const auto now = std::chrono::steady_clock::now();

			//稳定的临界点
			const auto steady_time = m_last_time + time_length;

			//已经稳定
			if(now >= steady_time)
			{
				//剩余时间为零
				const auto _rest_time = std::chrono::milliseconds{0};

				//转换
				const auto __rest_time = std::chrono::duration_cast<rest_time_T>(_rest_time);

				//给出
				rest_time = __rest_time;

				//给出
				return true;
			}
			//还未稳定
			{
				//剩余时间
				const auto _rest_time = steady_time - now;

				//转换
				const auto __rest_time = std::chrono::duration_cast<rest_time_T>(_rest_time);

				//给出
				rest_time = __rest_time;

				//给出
				return false;
			}
		}
		//特化
		bool is_enough_steady(std::chrono::milliseconds time_length) const
		{
			//准备个临时量
			std::chrono::milliseconds no_used{};

			//转发
			return is_enough_steady(time_length, no_used);
		}
		//重新开始检查
		void reset() const
		{
			m_last_value.reset();
		}
	};
}