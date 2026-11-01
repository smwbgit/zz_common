#pragma once

#include <windows.h>
#include <utils/thread/AtomicObject.hpp>
#include <utils/thread/Flag.hpp>
#include <utils/queue.hpp>
#include <utils/windows/ui/window/message_loop.hpp>
#include <utils/this_thread.hpp>
#include <utils/time/StartPosition.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_2
{
	//提供消息循环线程
	class Thread final
	{
	private:
		std::unique_ptr<std::thread> m_thread;
		utils::thread::Flag m_thread_exit_flag;
		utils::thread::AtomicObject<std::queue<std::function<void()>>> m_tasks;
	public:
		Thread()
		{
			//启动线程
			m_thread = std::make_unique<std::thread>([this](){ thread_proc(); });
		}
		~Thread()
		{
			//结束循环
			m_thread_exit_flag.set_true();

			//等待结束
			m_thread->join();

			//简单检查: 没有要创建的窗口
			utils_assert_abort(m_tasks.lock()->empty());
			
			//简单检查: 没有窗口存在
			utils_assert_abort(0 == utils::windows::ui::window::thread::count_thread_window());
		}
	public:
		Thread(const Thread &) = delete;
		Thread(Thread &&) = delete;
		Thread & operator =(const Thread &) = delete;
		Thread & operator =(Thread &&) = delete;
	public:
		//在窗口线程中执行指定函数
		//note: 在窗口线程/其他线程中，都能调用。当在窗口线程中被调用时，会退化为一次普通的函数调用
		void invoke_and_wait(std::function<void()> func)
		{
			//目前在窗口线程中
			if(std::this_thread::get_id() == m_thread->get_id())
			{
				//正常调用即可
				func();
			}
			//目前在其他线程中
			else
			{
				//准备数据：完成状态
				utils::thread::Flag flag;

				//添加任务
				m_tasks.lock()->push([func, &flag]()
				{
					//调用
					func();

					//更新状态：任务已完成
					flag.set_true();
				});

				//等待任务完成
				flag.wait();
			}
		}
		//在窗口线程中等待，每轮进行一次检测，直到指定条件满足
		//timeout_time_length: 超时时长。如果设置，那么如果超过此时长，则抛出异常
		void wait_in_window_thread(std::function<bool()> is_finish,
			std::optional<std::chrono::milliseconds> timeout_time_length)
		{
			utils::time::StartPosition time_start_position;
			while(1)
			{
				//查询结果
				utils::thread::Flag finished;
				invoke_and_wait([is_finish, &finished]()
				{
					if(is_finish())
						finished.set_true();
				});

				//已完成
				if(finished) break;

				//慢慢的查询
				utils::this_thread::yield();

				//超时，抛出异常
				if(timeout_time_length)
				{
					if(time_start_position.is_enough_passed(*timeout_time_length))
						utils_throw();
				}
			}
		}
		//当前是否在此线程中
		bool is_in() const
		{
			return std::this_thread::get_id() == m_thread->get_id();
		}
	private:
		//线程函数
		void thread_proc()
		{
			utils::windows::ui::window::message_loop::MessageLoop message_loop;
			//在空闲时创建窗口
			message_loop.m_meet_idle = [this]() {
				while(!m_tasks.lock()->empty())
				{
					auto task = utils::queue(*m_tasks.lock()).pop();
					task();
				}  };
			message_loop.m_thread_exit_flag = &m_thread_exit_flag;
			message_loop.start();
		}
	};
}