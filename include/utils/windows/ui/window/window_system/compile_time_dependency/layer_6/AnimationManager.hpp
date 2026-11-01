#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_5/Window.hpp>
#include <utils/Index2D.hpp>
#include <utils/ui/animation.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_6
{
	//向前声明
	class Window;

	namespace position_adjust
	{
		//动画时间调整
		class PositionAdjust
		{
		private:
			std::function<double(double position)> m_position_adjust;
		public:
			PositionAdjust() = default;
			PositionAdjust(std::function<double(double position)> position_adjust)
				: m_position_adjust{position_adjust}
			{
			}
		public:
			//计算调整后的时间位置
			double operator()(double position) const
			{
				//没有指定变换
				if(!m_position_adjust)
				{
					//直接给出原值
					return position;
				}

				//进行计算
				return m_position_adjust(position);
			}
		};

		//从匀速运动,变为,匀减速运动
		//v_end: 终止速度.为1时,退化为匀速运动
		inline PositionAdjust 匀减速运动(double v_end)
		{
			return {[v_end](double position)->double
			{
				//ai生成后整理

				double t = position;
				double v0 = 2.0 - v_end;      // 初速度
				double a  = 2.0 * (v_end - 1.0); // 加速度
				return v0 * t + 0.5 * a * t * t;
			}};
		}
	}

	//动画管理器
	class AnimationManager
	{
	public:
		//便利声明
		using PositionAdjust = position_adjust::PositionAdjust;
	private:
		layer_5::Window & m_window;
		utils::ui::animation::AnimationManager<decltype(m_window.m_timer_manager)> m_animation_manager;
	public:
		//匀速移动到指定偏移
		void move_to_by_offset(std::chrono::milliseconds time_length, utils::Index2D offset, PositionAdjust position_adjust)
		{
			//不需要偏移
			if(offset.is_all_zero())
			{
				//直接返回
				return;
			}

			//添加一个动画
			m_animation_manager.add_animation(time_length,
				[this, offset, position_adjust, last_offset = std::make_shared<utils::Index2D>()](double position)
			{
				//计算调整后的,时间位置
				const auto position_2 = position_adjust(position);

				//计算此时间片,新增偏移
				const auto small_offset = [&]()
				{
					//当前的偏移
					const auto offset_2 = offset * position_2;

					//算出新增偏移
					const auto small_offset = offset_2 - *last_offset;

					//更新状态
					*last_offset = offset_2;

					//给出
					return small_offset;
				}();

				//进行偏移
				m_window.get_s_hwnd().set_position_by_offset(small_offset);
			});
		}
		//淡出
		void disappear_by_alpha(std::chrono::milliseconds time_length)
		{
			//获取开始时的透明度
			const auto alpha = m_window.get_s_hwnd().get_watermark_alpha();

			//添加一个动画
			m_animation_manager.add_animation(time_length,
				[this, alpha](double position)
			{
				//新的透明度
				const auto alpha_2 = (1 - position) * alpha;

				//进行设置
				m_window.get_s_hwnd().set_watermark_alpha(alpha_2);
			});
		}
	private:
		friend class layer_6::Window;
	private:
		AnimationManager(layer_5::Window & window)
			: m_window{ window }
			, m_animation_manager{m_window.m_timer_manager}
		{
		}
	};
}