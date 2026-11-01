#pragma once

#include <vector>

namespace utils::ui::fps
{
	//计算fps
	//note: ai生成后整理
	class Fps
	{
	private:
		using Clock = std::chrono::steady_clock;
	private:
		Clock::time_point m_last_time = Clock::now();
		u8 m_frame_count = 0;
		double m_fps = 0.0;
	private:
		// 每隔 0.5 秒更新一次显示值，避免数字乱跳
		static constexpr auto UPDATE_INTERVAL = std::chrono::milliseconds(500);
	public:
		// 每帧调用一次
		void meet_one_frame()
		{
			//更新计数
			m_frame_count++;

			//当前时间
			const auto now = Clock::now();

			//距上次更新,过了多久
			const auto passed_time = now - m_last_time;

			//足够久,需要再次计算fps
			if (passed_time >= UPDATE_INTERVAL)
			{
				//简单转换
				const double sec = std::chrono::duration<double>(passed_time).count();

				//计算fps
				m_fps = m_frame_count / sec;

				//更新状态
				m_frame_count = 0;
				m_last_time = now;
			}
		}
		//获取fps
		double get() const
		{
			//给出
			return m_fps;
		}
	};
}