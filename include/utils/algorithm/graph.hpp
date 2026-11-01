#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <utils/set.hpp>
#include <utils/queue.hpp>

//图
namespace utils::algorithm::graph
{
	//广度优先遍历
	template<typename T, typename T_container = std::set<T>>
	class BreadthFirstSearch
	{
		//请指定
	public:
		T_container m_entry;	//note: 可以为空
		std::function<T_container/*所有新的后继结点*/(T node)> m_meet_one_node;

	private:
		bool m_is_stop{};

	public:
		void start()
		{
			//简单检查
			utils_assert(m_meet_one_node);

			std::queue<T> wait;
			std::set<T> visited;
			utils::queue(wait).push_many(m_entry);
			while(!wait.empty())
			{
				const auto node = utils::queue(wait).pop();

				if(utils::set(visited).is_exist(node)) continue;

				const auto next = m_meet_one_node(node);
				utils::set(visited).insert_not_exist(node);
				utils::queue(wait).push_many(next);

				if(m_is_stop) break;
			}
		}
		//不再继续遍历
		void stop()
		{
			m_is_stop = true;
		}
	};
#if 0
	//深度优先遍历
	template<typename T, typename T_container = std::set<T>>
	class DepthFirstSearch
	{
		//请指定
	public:
		std::optional<T> m_entry;	//note: 不可为空
		std::function<T_container/*所有新的后继结点*/(T node)> m_meet_one_node;

	private:
		bool m_is_stop{};

	public:
		void start()
		{
			//简单检查
			utils_assert(m_entry);
			utils_assert(m_meet_one_node);

			std::queue<T> wait;
			std::set<T> visited;
			utils::queue(wait).push_many(m_entry);
			while(!wait.empty())
			{
				const auto node = utils::queue(wait).pop();

				if(utils::set(visited).is_exist(node)) continue;

				const auto next = m_meet_one_node(node);
				utils::set(visited).insert_not_exist(node);
				utils::queue(wait).push_many(next);

				if(m_is_stop) break;
			}
		}
		//不再继续遍历
		void stop()
		{
			m_is_stop = true;
		}
	};
#endif
}