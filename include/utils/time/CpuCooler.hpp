#pragma once

#include <chrono>
#include <thread>
#include <optional>
#include <utils/assert.hpp>
#include <utils/time/StartPosition.hpp>

namespace utils::time
{
	//在连续做完一个重cpu任务后，让cpu休息一下
	class CpuCooler
	{
	public:
		struct s_param
		{
			struct s_optional
			{
				std::optional<std::chrono::steady_clock::duration> max_rest_time;	//每次的最长休息时间
			};
			s_optional optional;
		};
		s_param m_param;

	private:
		//cpu开始工作的时间点
		utils::time::StartPosition m_start_position;

	public:
		//开始休息
		//note：根据过去的时间，来决定休息时间
		void sleep_to_cool()
		{
			//获得已过去的时间
			const auto passed_time = m_start_position.get_passed_time();

			//计算出要休息的时间
			const auto rest_time = [&]()
			{
				//休息十分之一的时间
				auto rest_time = passed_time / 10;

				//获取最大休息时间
				const auto max_rest_time = [&]()
				{
					//默认值
					const auto default_value = std::chrono::minutes{10};

					//用户指定 或者 使用默认值
					return m_param.optional.max_rest_time.value_or(default_value);
				}();

				//休息时间过长，则变短
				if (rest_time > max_rest_time)
					rest_time = max_rest_time;

				//给出
				return rest_time;
			}();

			//开始休息
			std::this_thread::sleep_for(rest_time);

			//重新计时
			m_start_position.reset();
		}
	};
}