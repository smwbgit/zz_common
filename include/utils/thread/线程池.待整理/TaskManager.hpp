#pragma once

#include <vector>
#include <functional>
#include <atomic>
#include "utils/threads/ThreadManager.hpp"

namespace utils::threads
{
	//使用指定数量的线程组, 完成更多量的任务
	class TaskManager
	{
	public:
		//启动某一种类型的任务
		class s_task
		{
		private:
			TaskManager* m_this{};
		public:
			s_task(TaskManager* _this)
				: m_this{_this}
			{
			}

			//把容器里的元素依次给出, 返回每个元素的输出
			//meet_one_element: 在worker线程上运行的函数, 传入容器里的一个元素, 传出该元素的输出
			template<typename T, typename Result_T>
			std::vector<Result_T> visit_all(
				_In_ const std::vector<T> & container,
				_In_ std::function<Result_T(const T & element)> meet_one_element)
			{
				std::vector<Result_T> re;
				re.resize(container.size());
				std::atomic<uint64_t> element_index{};

				struct s_data
				{
				};
				utils::threads::ThreadManager<s_data> manager;
				manager.create(m_this->m_thread_n, [&](auto & entry)
				{
					while(1)
					{
						auto index = element_index.fetch_add(1);
						if (index >= container.size())
							break;

						re[index] = meet_one_element(container[index]);
					}
				});
				manager.join_all();

				return re;
			}
			
			//从容器里选出符合条件的元素
			//meet_one_element: 在worker线程上运行的函数, 传入容器里的一个元素, 传出该元素是否符合条件
			//return: 所有符合的元素
			//example:
			//	utils::threads::TaskManager manager{16};
			//	std::vector<uint8_t> right = manager.task.pick<uint8_t>(__bytes, [&](const uint8_t & input_data) -> bool
			//	{
			//		return test_this(input_data);
			//	});
			template<typename T>
			std::vector<T> pick(
				_In_ const std::vector<T> & container,
				_In_ std::function<bool(const T & element)> meet_one_element)
			{
				auto test_result = visit_all<T, bool>(container, [&](const T & element) -> bool
				{
					return meet_one_element(element);
				});

				//整理结果
				std::vector<T> re;
				for (uint64_t i = 0; i < test_result.size(); i++)
				{
					if (test_result[i])
						re.push_back(container[i]);
				}
				return re;
			}
		};

	public:
		s_task task{this};

	private:
		uint64_t m_thread_n{};

	public:
		//thread_n: 最大并行数
		TaskManager(_In_ const uint64_t thread_n)
			: m_thread_n{thread_n}
		{
		}
	};
}