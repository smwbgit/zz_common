#pragma once

#include <optional>
#include <utils/ui/TimerManager.hpp>
#include <utils/NextId.hpp>
#include <utils/map.hpp>
#include <utils/integer_type.hpp>

//动画动作
namespace utils::ui::animation
{
	//一个动画动作
	class Animation
	{
	private:
		std::chrono::steady_clock::time_point m_begin_time = std::chrono::steady_clock::now();
		std::optional<double> m_last_position;
	public:
		//获得当前时间片,处于,整段时间的什么位置
		//whole_time_length: 整段时间的长度
		//return: 当和上次调用时,返回的,位置相同时,返回空值.
		std::optional<double> get_time_position(std::chrono::milliseconds whole_time_length)
		{
			//获取当前时间
			const auto now = std::chrono::steady_clock::now();

			//简单检查
			utils_assert(now >= m_begin_time);

			//获取时间长度
			const auto length = now - m_begin_time;

			//转为毫秒
			const auto length_2 = std::chrono::duration_cast<std::chrono::milliseconds>(length);

			//简单检查
			utils_assert(length_2.count() >= 0);
			utils_assert(whole_time_length.count() > 0);

			//计算位置
			const auto position = 1. * length_2.count() / whole_time_length.count();

			//当超出1时,算为1
			const auto position_2 = position > 1 ? 1 : position;

			//简单检查
			utils_assert(position_2 >= 0);
			utils_assert(position_2 <= 1);

			//和上次的时间位置相同
			if(m_last_position && *m_last_position == position_2)
			{
				//给出空值
				return {};
			}

			//简单检查
			utils_assert(!m_last_position || *m_last_position <= position_2);

			//更新状态
			m_last_position = position_2;

			//给出
			return {position_2};
		}
	};

	//动画管理器
	template<utils::ui::concepts::TimerManager TimerManager>
	class AnimationManager
	{
	private:
		utils::NextId<u8> m_next_id;
		std::map<u8, std::unique_ptr<Animation>> m_all_animation;
		TimerManager & m_timer_manager;
	public:
		AnimationManager(TimerManager & timer_manager)
			: m_timer_manager{ timer_manager }
		{
		}
	public:
		//添加一个动画
		//time_length: 动画的总用时. 0为直接显示动画的最后一帧
		//meet_time_piece: 每帧回调
		void add_animation(std::chrono::milliseconds time_length, std::function<void(double position)> meet_time_piece)
		{
			//简单检查
			utils_assert(time_length.count() >= 0);

			//直接完成动画
			if(time_length.count() == 0)
			{
				//进行一帧动画
				meet_time_piece(1);

				//完成
				return;
			}

			//生成新的id
			const auto id = m_next_id.get();

			//准备数据
			auto animation = std::make_unique<Animation>();
			auto & animation_2 = *animation;

			//添加定时器
			m_timer_manager.add_timer(std::chrono::milliseconds{ 1 }, [this, &animation_2, meet_time_piece, id, time_length](bool & is_kill_timer)
			{
				//计算时间位置
				std::optional<double> position = animation_2.get_time_position(time_length);

				//和之前的位置相同
				if(!position)
				{
					//画面已有,不再继续
					return;
				}

				//简单检查
				utils_assert(*position >= 0);
				utils_assert(*position <= 1);

				//进行一帧动画
				meet_time_piece(*position);

				//动画已结束
				if(*position == 1)
				{
					//关闭定时器
					is_kill_timer = true;

					//删除自己的表项
					utils::map(m_all_animation).remove(id);
				}
			});

			//保存数据
			utils::map(m_all_animation).insert_not_exist(id, std::move(animation));
		}
	};
}