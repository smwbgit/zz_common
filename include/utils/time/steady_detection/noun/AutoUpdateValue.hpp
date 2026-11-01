#pragma once

#include <utils/time/steady_detection/SteadyDetection.hpp>
#include <concepts>

namespace utils::time::steady_detection::noun
{
	//自动更新数据
	template<typename T>
	class AutoUpdateValue
	{
	private:
		utils::time::steady_detection::SteadyDetection<T> m_steady_detection;
	public:
		//是否足够稳定
		bool is_enough_steady(std::chrono::milliseconds time_length) const
		{
			//更新当前值
			update_value();

			//转发
			return m_steady_detection.is_enough_steady(time_length);
		}
		//是否足够稳定
		//rest_time: 剩余时间
		template<typename rest_time_T>
			requires requires {
				std::chrono::duration_cast<rest_time_T>(std::declval<std::chrono::milliseconds>());
			}
		bool is_enough_steady(std::chrono::milliseconds time_length, rest_time_T & rest_time) const
		{
			//更新当前值
			update_value();

			//转发
			return m_steady_detection.is_enough_steady(time_length, rest_time);
		}
	private:
		//更新当前值
		void update_value() const
		{
			//获取当前值
			const auto current_value = get_current_value();

			//更新当前值
			m_steady_detection.update_value(current_value);
		}
	private:
		//获取当前值
		virtual T get_current_value() const = 0;
	};
}