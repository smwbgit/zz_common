#pragma once

#include <windows.h>
#include <utils/thread/Flag.hpp>
#include <utils/windows/ui/window/thread.hpp>

namespace utils::windows::ui::window::message_loop
{
	class MessageLoop
	{
	public:
		utils::thread::Flag* m_thread_exit_flag{};
		std::function<void()> m_meet_idle;
	public:
		int start()
		{
			while(1)
			{
				check_exit_flag();

				MSG msg{};
				if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
						return on_exit((int)msg.wParam);
            
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
				else
				{
					on_idle();
            
					//短暂休眠以避免过度占用CPU
					Sleep(1);
				}
			}
		}
	private:
		void on_idle()
		{
			if (m_meet_idle)
				m_meet_idle();
		}
		void check_exit_flag()
		{
			if (m_thread_exit_flag && m_thread_exit_flag->is_true())
				PostQuitMessage(0);
		}
		int on_exit(int exit_code)
		{
			//简单检查: 当前线程没有任何窗口
			utils_assert(0 == utils::windows::ui::window::thread::count_thread_window());

			return exit_code;
		}
	};
}
