#pragma once

#include <mutex>
#include <atomic>
#include <functional>
#include <utils/assert.hpp>
#include <utils/this_thread.hpp>

namespace utils::thread
{
	//对std::thread的简单包装。主要提供，让其安全退出的通用逻辑
	//note: 使用时，要在线程完成初始化后，告诉一下（通过调用init_be_ready）。以及，在线程运行过程中，
	//		时刻检查is_should_finish函数的返回值，当为true时，开始退出线程。
	class Thread
	{
	private:
		std::atomic_bool m_is_init_ready{false};
		std::atomic_bool m_is_finished{false};
		std::atomic_bool m_is_should_finish{false};
		std::thread m_thread;
	public:
		bool m_is_safe_exiting = true;	//是否在对象析构时，等待线程自己退出
	public:
		Thread(std::function<void(const std::atomic_bool & is_should_finish)> func)
			: m_thread{[func, this]()
			{
				func(m_is_should_finish);

				m_is_finished = true;
			}}
		{
		}
		//特化
		Thread(std::function<void()> func)
			: m_thread{[func, this]()
			{
				func();

				m_is_finished = true;
			}}
		{
		}
		~Thread()
		{
			if(!is_finished() && m_is_safe_exiting)
			{
				wait_finished();
			}
		}
		//等待线程初始化完成
		void wait_init_ready()
		{
			//已经完成
			if(m_is_init_ready)
				return;

			//等待结束
			utils::this_thread::wait_true(m_is_init_ready);
		}
		//线程完成初始化
		void init_be_ready()
		{
			//简单检查
			utils_assert(!m_is_init_ready);

			//更新状态
			m_is_init_ready = true;
		}
		//等待线程结束
		void wait_finished()
		{
			//已经结束
			if(m_is_finished)
				return;

			//设置标志
			m_is_should_finish = true;

			//等待结束
			utils::this_thread::wait_true(m_is_finished);
		}
		//线程是否结束
		bool is_finished() const
		{
			return m_is_finished;
		}
		//是否应该结束
		bool is_should_finish() const
		{
			return m_is_should_finish;
		}
	};
}