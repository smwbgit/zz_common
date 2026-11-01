#pragma once

#include <any>
#include "utils/string.hpp"
#include "utils/serialize/any.hpp"

#include "DataTree.hpp.inline.SubTree.h.hpp"

namespace utils::data_tree::text
{
	//向前声明
	inline void to_text(_Out_ std::string & text, _In_ utils::data_tree::DataTree & tree);
	inline void from_text(_Out_ utils::data_tree::DataTree & tree, _In_ const std::string & text);
}

namespace utils::data_tree
{
	namespace inner::DataTree
	{
		//文件节点, i.e.数据树上的一个叶子节点
		class File
		{
		private:
			std::any m_value;
		public:
			template<typename T>
			void get(T & value) const
			{
				value = std::any_cast<T>(m_value);
			}
			template<typename T>
			void set(const T & value)
			{
				m_value = value;
			}
			//序列化
			template<bool is_saving, typename T>
			void serialize(T& file)
			{
				file & m_value;
			}
			std::any & get_m_value()
			{
				return m_value;
			}
		};
		//文件夹节点
		class Folder
		{
		public:
			uint64_t m_count{};
		public:
			//增加一个引用
			void reference()
			{
				m_count++;
			}
			//减少一个引用
			void dereference()
			{
				//简单检查
				utils_assert(m_count > 0);

				m_count--;
			}

			//序列化
			template<bool is_saving, typename T>
			void serialize(T& file)
			{
				file & m_count;
			}
		};
	}

	//数据树
	class DataTree : public inner::SubTree
	{
	public:
		using SubTree = inner::SubTree;

		//存储格式
		enum class StorageFormat
		{
			Text,	//文本模式
			Binary,	//二进制模式
		};
	private:
		utils::map<utils::string, inner::DataTree::File> m_files;			//文件节点
		utils::map<utils::string, inner::DataTree::Folder> m_folders;		//文件夹节点
	private:
		friend class SubTree;
	public:
		DataTree()
			: SubTree{this, ""}
		{
		}
		//清空
		void clear()
		{
			m_files.clear();
			m_folders.clear();
		}
		//序列化
		template<bool is_saving, typename T>
		void serialize(T& file)
		{
			//版本号
			//note: 防止读取到不同版本的格式
			{
				const uint64_t version = 230512;
				if (is_saving)
					file & (uint64_t &)version;
				else
				{
					uint64_t _version{};
					file & _version;

					//简单检查
					utils_assert(_version == version);
				}
			}

			//数据
			file & m_files;
			file & m_folders;
		}

#pragma region utils::Storage支持
		//载入
		template<typename T>
		void load_storage(T & input, _In_ StorageFormat storage_format)
		{
			switch (storage_format)
			{
			case StorageFormat::Text:
			{
				std::string text;
				input.read_whole_file(text);

				utils::data_tree::text::from_text(*this, text);
				return;
			}
			case StorageFormat::Binary:
			{
				input.serializer & (*this);
				return;
			}
			default:
				utils_throw();
			}
		}
		//保存
		template<typename T>
		void save_storage(T & output, _In_ StorageFormat storage_format)
		{
			switch (storage_format)
			{
			case StorageFormat::Text:
			{
				std::string text;
				utils::data_tree::text::to_text(text, *this);

				output.text.write(text);
				return;
			}
			case StorageFormat::Binary:
			{
				output.serializer & (*this);
				return;
			}
			default:
				utils_throw();
			}
		}
#pragma endregion
#pragma region 帮助函数
	public:
		class s_help_function
		{
		private:
			DataTree* m_this{};
			friend class DataTree;
			s_help_function(DataTree* _this) : m_this{_this} {}
		public:
			//获取一个名字的所有前边的路径
			//e.g.  "a/b/c/d" -> {"a", "a/b", "a/b/c"}
			static void get_folders(_Out_ std::vector<std::string> & folders, _In_ const std::string & name)
			{
				auto ones = ((const utils::string &)name).string_view().split('/');

				folders.clear();
				for (const auto i : utils::iterator_index(ones.size() - 1))
				{
					std::string f;
					for (const auto j : utils::iterator_index(i + 1))
					{
						if (j > 0)
							f += '/';
						f += ones[j];
					}
					folders.push_back(f);
				}
			}
			//获取一个名字中的所有名字
			//e.g.  "a/b/c/d" -> {"a", "b", "c", "d"}
			static void get_names(_Out_ std::vector<std::string> & names, _In_ const std::string & name)
			{
				auto ones = ((const utils::string &)name).string_view().split('/');

				names.clear();
				for (const auto one : ones)	//todo 看看怎么省略这个push_back循环
				{
					names.push_back(one);
				}
			}
			const utils::map<utils::string, inner::DataTree::File> & get_m_files()
			{
				return m_this->m_files;
			}
			//获得文件的底层表示 i.e. std::any
			std::any & get_underlying_representation(_In_ const std::string & name)
			{
				return m_this->m_files.get_existed_value(name).get_m_value();
			}
		};
		s_help_function help_function{this};
#pragma endregion
	};
}
namespace utils
{
	//导出
	using DataTree = utils::data_tree::DataTree;
}

#include "utils/data_tree/text.hpp"

#include "DataTree.hpp.inline.SubTree.cpp.hpp"
