#pragma once

#include <windows.h>
#include <utils/map.hpp>
#include <utils/IdGiver.hpp>

namespace utils::windows::ui::window::timer
{
	/*
		usage:
			case WM_CREATE:
				m_timer_manager.add_timer(1000, [this]()
				{
					m_ui_color_i++;
				});
				break;

			case WM_TIMER:
				m_timer_manager->meet_timer_message(wParam, lParam);
				break;
	*/
	class TimerManager
	{
	private:
		HWND m_hwnd{};
		utils::IdGiver<uint64_t> m_id_giver;
		std::map<uint64_t, std::function<void()>> m_handles;
	public:
		TimerManager(HWND hwnd)
			: m_hwnd{hwnd}
		{
			//我们使用消息机制，没有用到回调函数, 关闭这段代码
#if 0
			//不让windows抑制定时器回调函数中的异常
			BOOL value = FALSE;
			utils_assert(SetUserObjectInformationW(
				GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION,
				&value, sizeof(value);
#endif
		}
		//return: timer_id
		uint64_t add_timer(uint32_t ms, std::function<void()> meet_timer_expired)
		{
			//创建定时器
			const auto timer_id = m_id_giver.get_one_id();
			utils_assert(SetTimer(m_hwnd, timer_id, ms, NULL));

			//保存回调函数
			utils::map(m_handles).insert_not_exist(timer_id, meet_timer_expired);
			return timer_id;
		}
		void remove_timer(uint64_t timer_id)
		{
			//todo 看看这样删除会不会导致，消息队列中，还有此id的Timer消息，等到这个消息被处理时，会不会有问题

			//删除回调函数
			utils::map(m_handles).remove(timer_id);

			//删除回调函数
			utils_assert(KillTimer(m_hwnd, timer_id));
		}
		void meet_timer_message(WPARAM wParam, LPARAM lParam)
		{
			//定时器id
			uint64_t timer_id = wParam;

			//查找回调函数
			auto & handle = utils::map(m_handles).get_existed_value(timer_id);
			
			//进行处理
			handle();
		}
	};
}