
#pragma region utils::data_tree::inner::SubTree 的声明

namespace utils::data_tree
{
	class DataTree;
}

namespace utils::data_tree::inner
{
	//表示一个子树，具有自己的路径前缀和DataTree的引用
	class SubTree
	{
	private:
		using DataTree = utils::data_tree::DataTree;
	private:
		DataTree* m_tree{};
		utils::string m_prefix;	//路径前缀
	protected:
		SubTree(DataTree* tree = nullptr, const std::string & prefix = "");
	public:
#pragma region 子树
		//获得一个子树。如果子树不存在，并[不]报错
		SubTree get_sub_tree(const std::string & name);
		//特化
		SubTree operator[](const std::string & name);
		//获得父节点
		SubTree get_parent();
		//特化: 获得指定远处的父节点
		//example:
		//	get_parent(0)相当于无操作
		//	get_parent(1)相当于get_parent()
		//	get_parent(2)相当于get_parent().get_parent()
		SubTree get_parent(_In_ uint64_t times);
#if 0
		//拼接路径。返回一个m_prefix + '/' + name字符串
		//is_easy_path: 当name的第一个字符是'/'时，是否去掉这个多出的'/'
		std::string get_full_name(const std::string & name, bool is_easy_path) const;
#endif
#pragma endregion
#pragma region 读取
#pragma region 一个数据
		//读取一个数据。如果数据不存在，则报错
		template<typename T>
		void get(const std::string & name, T && value) const;
		//特化：如果不存在，则无操作
		//return: 是否存在
		template<typename T>
		bool get_optional(const std::string & name, T & value) const;
#pragma endregion
#pragma region 数组
		//获取数组。如果数组不存在，则报错
		//note: 如果数组大小为0，则当作它不存在
		//meet_one_element: 接收数组中的一个元素
		//index: 指出是数组中的第几个元素
		void get_array(const std::string & name, std::function<void(uint64_t index, SubTree)> meet_one_element);	//todo 看看这里能不能把SubTree换成DataTree::SubTree
		//特化：如果不存在，则无操作
		//return: 是否存在
		bool get_optional_array(const std::string & name, std::function<void(uint64_t index, SubTree)> meet_one_element);
#pragma endregion
#pragma endregion
#pragma region 写入
#pragma region 一个数据
		//写入一个数据。如果数据存在，则覆盖
		//note: 数据的路径上的每个节点，不能是文件节点
		//note: 数据的本身路径，不能是文件夹节点
		template<typename T>
		void set(const std::string & name, const T & value);
#pragma endregion
#pragma endregion
#pragma region 判断
		//是否是文件节点
		//is_easy_path: 参见get_full_name()
		bool is_file(const std::string & name, bool is_easy_path = false) const;
		//是否是文件夹节点
		bool is_folder(const std::string & name) const;
#pragma endregion
	};

}
#pragma endregion
