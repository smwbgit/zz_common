#pragma once

#include "utils/FileReader.hpp"
#include "utils/real_path.hpp"
#include "utils/FileWriter.hpp"

namespace utils
{
	//表示一个存储位置，比如一个文件
	//example:	file://cur:/1.txt	//当前目录的1.txt
	//			file://tmp:/1.txt	//临时目录的1.txt
	//			file://exe:/1.txt	//exe所在目录的1.txt
	//			file://c:/1.txt
	class StorageLocation
	{
	public:
		enum class Type
		{
			File,	//文件
		};
	private:
		Type m_type{};
		std::filesystem::path m_filename;
	public:
		StorageLocation(utils::string_view location)
		{
			parse(location);
		}
		Type get_type()
		{
			return m_type;
		}
		//获取直接可用的完整文件路径
		std::filesystem::path get_filename()
		{
			//简单检查
			utils_assert(m_type == Type::File);

			return m_filename;
		}
	private:
		//解析字符串
		void parse(utils::string_view location)
		{
			//文件
			if (location.starts_with("file://", true))
			{
				m_type = Type::File;

				//当前目录
				if (location.starts_with("cur:/", true))
				{
					utils_throw();	//todo
				}
				//临时目录
				else if (location.starts_with("tmp:/", true))
				{
					utils_throw();	//todo
				}
				//exe所在目录
				else if (location.starts_with("exe:/", true))
				{
					m_filename = utils::real_path::get_exe_directory() / std::string_view{location};
				}
				//普通目录
				else
				{
					m_filename = std::string_view{location};
				}
			}
			else
			{
				utils_throw();
			}
		}
	};

	//表示一个存储空间。主要负责把存储空间的内容传给T，以及把T传来的数据保存在存储空间里
	template<typename T>
	class Storage : public T
	{
	private:
		utils::StorageLocation m_location;
	public:
		Storage(_In_ utils::string location)
			: m_location{location}
		{
		}
		//读取
		template<typename ...Ts>
		void load_storage(_In_ Ts && ...args)
		{
			switch (m_location.get_type())
			{
			case utils::StorageLocation::Type::File:
			{
				utils::FileReader input{m_location.get_filename()};

				//转发
				((T*)this)->load_storage(input, std::forward<Ts>(args)...);

				//简单检查
				input.readed_all();
				return;
			}
			default:
				utils_throw();
			}
		}
		//保存
		template<typename ...Ts>
		void save_storage(_In_ Ts && ...args)
		{
			switch (m_location.get_type())
			{
			case utils::StorageLocation::Type::File:
			{
				utils::FileWriter output{m_location.get_filename()};

				//转发
				((T*)this)->save_storage(output, std::forward<Ts>(args)...);
				return;
			}
			default:
				utils_throw();
			}
		}
	};
}
