#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include "utils/iterator.hpp"

namespace utils::threads
{
	//创建指定大小的线程数组
	template<typename UserData_T>
	class ThreadManager
	{
	public:
		struct s_entry
		{
			std::thread thread;
			const uint64_t index{};
			UserData_T user_data;
		};
	public:
		std::vector<std::unique_ptr<s_entry>> m_threads;
	public:
		//创建线程数组
		//thread_n: 数组大小
		//thread_func: 线程函数，参数可用来访问用户数据
		void create(uint64_t thread_n, std::function<void(s_entry &)> thread_func)
		{
			m_threads.resize(thread_n);
			for (const auto i : utils::iterator_index(m_threads.size()))
			{
				auto wrap_func = [thread_func, this, i]()
				{
					thread_func(*m_threads[i]);
				};
				auto e = new s_entry{std::thread{wrap_func}, i, {}};
				m_threads[i].reset(e);
			}
		}
		//等待全部线程结束
		void join_all()
		{
			for (auto & t : m_threads)
				t->thread.join();
		}
	};
}