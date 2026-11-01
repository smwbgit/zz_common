

#pragma region utils::data_tree::inner::SubTree 的实现

namespace utils::data_tree::inner
{
	inline SubTree::SubTree(DataTree* tree, const std::string & prefix)
	{
		m_tree = tree;
		m_prefix = prefix;
	}
#pragma region 子树
	inline SubTree SubTree::get_sub_tree(const std::string & name)
	{
		SubTree sub_tree;
		sub_tree.m_tree = m_tree;
		sub_tree.m_prefix = m_prefix + '/' + name;
		return sub_tree;
	}
	inline SubTree SubTree::operator[](const std::string & name)
	{
		return get_sub_tree(name);
	}
	inline SubTree SubTree::get_parent()
	{
		//父节点的路径
		std::string_view prefix;
		{
			prefix = m_prefix;
			auto pos = prefix.rfind('/');

			//简单检查
			utils_assert(pos != prefix.npos);

			prefix = prefix.substr(0, pos);
		}

		SubTree sub_tree;
		sub_tree.m_tree = m_tree;
		sub_tree.m_prefix = prefix;
		return sub_tree;
	}
	inline SubTree SubTree::get_parent(_In_ uint64_t times)
	{
		SubTree sub_tree = *this;
		for (auto i : utils::iterator_index(times))
			sub_tree = sub_tree.get_parent();

		return sub_tree;
	}
#if 0
	inline std::string SubTree::get_full_name(const std::string & name, bool is_easy_path) const
	{
		if (is_easy_path)
		{
			//如果name前边有'/'，则拼接时不加'/'

			//简单检查
			utils_assert(name.empty() == false);

			//给出
			if (name[0] == '/')
				return m_prefix + name;
			else
				return m_prefix + '/' + name;
		}
		else
		{
			//给出
			return m_prefix + '/' + name;
		}
	}
#endif
#pragma endregion
#pragma region 读取
#pragma region 一个数据
	template<typename T>
	void SubTree::get(const std::string & name, T && value) const
	{
		utils_assert(get_optional(name, std::forward<T>(value)));
	}
	template<typename T>
	bool SubTree::get_optional(const std::string & name, T & value) const
	{
		auto _name = m_prefix + '/' + name;

		if (false == m_tree->m_files.exist(_name))
			return false;

		m_tree->m_files[_name].get(value);
		return true;
	}
#pragma endregion
#pragma region 数组
	inline void SubTree::get_array(const std::string & name, std::function<void(uint64_t index, SubTree)> meet_one_element)
	{
		//转发
		utils_assert(get_optional_array(name, meet_one_element));
	}
	inline bool SubTree::get_optional_array(const std::string & name, std::function<void(uint64_t index, SubTree)> meet_one_element)
	{
		//不存在，则返回
		uint64_t size{};
		if (false == get_optional(name + "/$array_size", size))
			return false;
		if (size == 0)
			return false;

		for (const uint64_t i : utils::iterator_index(size))
		{
			meet_one_element(i, get_sub_tree(name + "/[" + std::to_string(i) + "]"));
		}
		return true;
	}
#pragma endregion
#pragma endregion
#pragma region 写入
#pragma region 一个数据
	template<typename T>
	void SubTree::set(const std::string & name, const T & value)
	{
		//如果文件节点不存在，则先创建
		if (false == is_file(name))
		{
			//数据的本身路径，不能是文件夹节点
			utils_assert(false == is_folder(name));

			//数据的路径上的每个节点，不能是文件节点
			std::vector<std::string> _folders;
			{
				m_tree->help_function.get_folders(_folders, name);

				for (const auto & folder : _folders)
				{
					utils_assert(false == is_file(folder));
				}
			}

			//创建文件节点
			m_tree->m_files[m_prefix + '/' + name] = {};

			//更新状态: 路径上的每个文件夹节点，增加引用
			for (const auto & folder : _folders)
			{
				m_tree->m_folders[m_prefix + '/' + folder].reference();
			}
		}

		//保存
		m_tree->m_files[m_prefix + '/' + name].set(value);
	}
#pragma endregion
#pragma endregion
#pragma region 判断
	inline bool SubTree::is_file(const std::string & name, bool is_easy_path) const
	{
//		auto full_name = get_full_name(name, is_easy_path);
		auto full_name = m_prefix + '/' + name;

		return m_tree->m_files.exist(full_name);
	}
	inline bool SubTree::is_folder(const std::string & name) const
	{
//		auto _name = get_full_name(name, false);	//todo 看看用不用把false变为参数传入的
		auto _name = m_prefix + '/' + name;

		//如果不存在
		auto iter = m_tree->m_folders.find(_name);
		if (iter == m_tree->m_folders.end())
			return false;

		//如果计数为零
		if (iter->second.m_count == 0)
			return false;

		return true;
	}
#pragma endregion
}

#pragma endregion