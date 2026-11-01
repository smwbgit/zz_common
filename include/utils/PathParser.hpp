#pragma once

#include <functional>
#include <string>
#include <vector>
#include "utils/assert.hpp"
#include "utils/string.hpp"

namespace utils
{
	//字符串形式的路径的遍历器
	template<typename T>
	class PathParser
	{
	public:
		using try_get_child_t = std::function<bool(_Out_ T* & child, _In_ T& parent, _In_ const std::string & child_name)>;
		using visit_child_t = std::function<void(T& parent, const std::string & child_name, std::function<void(T& node)> meet_one_node)>;

		PathParser(try_get_child_t try_get_child, visit_child_t visit_child)
			: m_try_get_child{try_get_child}
			, m_visit_child{visit_child}
		{
		}


		//指定节点是否存在
		bool is_node_exist(T& root, const std::string & path)
		{
			//分割路径
			std::vector<std::string> names = split_path(path);

			//试着走到节点
			T* node{};
			for (const auto & name : names)
			{
				//简单检查
				utils_assert(name.empty() == false);

				if (node == nullptr)
				{
					//简单检查
					utils_assert(root.name == name);

					node = &root;
				}
				else
				{
					if (false == m_try_get_child(node, *node, name))
						return false;
				}
			}

			//找到该节点
			return true;
		}
		//获得叶子节点
		void get_leaf(T& root, const std::string & path, std::function<void(T& tag)> meet_leaf)
		{
			//分割路径
			std::vector<std::string> names = split_path(path);

			//遍历
			T* node{};
			for (const auto & name : names)
			{
				//简单检查
				utils_assert(name.empty() == false);

				if (node == nullptr)
				{
					//简单检查
					utils_assert(root.name == name);

					node = &root;
				}
				else
				{
					wrap_get_child(node, *node, name);
				}
			}

			//给出
			meet_leaf(*node);
		}

		//遍历指定位置，可能会有多个符合的节点
		void visit(T& root, const std::string & path, std::function<void(T& node)> meet_one_node)
		{
			//分割路径
			std::vector<std::string> names = split_path(path);

			//弹出最后一个单独处理
			auto last_name = *names.rbegin();
			names.pop_back();

			//走到父节点
			T* node{};
			for (const auto & name : names)
			{
				//简单检查
				utils_assert(name.empty() == false);

				if (node == nullptr)
				{
					//简单检查
					utils_assert(root.name == name);

					node = &root;
				}
				else
				{
					wrap_get_child(node, *node, name);
				}
			}
			if (node == nullptr)
			{
				//简单检查
				utils_assert(names.empty());

				node = &root;
			}

			//给出
			m_visit_child(*node, last_name, meet_one_node);
		}

	private:
		void wrap_get_child(_Out_ T* & child, _In_ T& parent, _In_ const std::string & child_name)
		{
			utils_assert(m_try_get_child(child, parent, child_name));
		}
		//分割路径
		std::vector<std::string> split_path(const std::string & path)
		{
			//简单检查
			utils_assert(path.empty() == false);
			utils_assert(path[0] != '/');
			utils_assert(path[path.length() - 1] != '/');

			//分割
			std::vector<std::string> names;
			utils::string_function::split(names, path, '/');
			return names;
		}

		try_get_child_t m_try_get_child;
		visit_child_t m_visit_child;
	};
}
