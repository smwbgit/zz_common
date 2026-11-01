#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_3/Window.hpp>
#include <utils/map.hpp>
#include <utils/ui/TimerManager.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4
{
	//向前声明
	class Window;

	class TimerManager
	{
	private:
		layer_3::Window & m_window;
		utils::NextId<uint32_t> m_timer_next_id;
		std::map<uint32_t, std::function<void(bool & is_kill_timer)>> m_timer_handlers;
	public:
		void add_timer(std::chrono::milliseconds time_length, std::function<void(bool & is_kill_timer)> meet_expired)
		{
			//简单检查
			utils_assert(time_length.count() > 0);
			utils_assert(m_window.get_hwnd());

			//准备数据
			const auto timer_id = m_timer_next_id.get();

			//添加定时器
			utils_assert(SetTimer(m_window.get_hwnd(), timer_id, time_length.count(), NULL));

			//保存回调函数
			m_timer_handlers[timer_id] = meet_expired;
		}
		//特化
		void add_timer(std::chrono::milliseconds time_length, std::function<void()> meet_expired)
		{
			//转发
			return add_timer(time_length, [meet_expired](bool & is_kill_timer)
			{
				meet_expired();
			});
		}
	private:
		friend class layer_4::Window;
	private:
		TimerManager(layer_3::Window & window)
			: m_window{ window }
		{
		}
		LRESULT meet_window_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			//简单检查
			utils_assert(hwnd == m_window.get_hwnd());

			//准备数据
			const auto timer_id = (uint32_t)wParam;

			//跳过id为0的。id为0时的消息，似乎是给系统的某部分使用的
			if (0 == timer_id)
				return DefWindowProcW(hwnd, uMsg, wParam, lParam);
			
			//定时器已被删除
			if(!utils::map(m_timer_handlers).is_exist(timer_id))
			{
				//直接返回
				return 0;
			}

			//准备数据
			const auto meet_expired = 
				utils::map(m_timer_handlers).get_existed_value(timer_id);

			//准备数据
			bool is_kill_timer{};

			//通知用户
			meet_expired(is_kill_timer);

			//删除定时器
			if(is_kill_timer)
			{
				//从系统中删除
				utils_assert(KillTimer(hwnd, timer_id));

				//从自己的表中删除
				utils::map(m_timer_handlers).remove(timer_id);
			}

			//完成
			return 0;
		}
	};

	// 简单检查
	static_assert(utils::ui::concepts::TimerManager<TimerManager>);
}