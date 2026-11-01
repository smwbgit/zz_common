#pragma once

#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>

namespace utils::inner::windows::dump
{
	//基类
	template<typename T>
	class Base
	{
	public:
		template<typename ...T_args>
		static void dump_to_file(const std::string & filename, T_args && ... args)
		{
			//收集信息
			auto data = T::gather(std::forward<T_args>(args)...);

			//保存
			utils::FileWriter output{filename};
			output.write(data);
		}
		template<typename ...T_args>
		static T load_file(const std::string & filename, T_args && ... args)
		{
			//读取文件
			utils::FileReader input{filename};
			auto data = input.read_all();

			//执行分析
			return T::analysis(std::move(data), std::forward<T_args>(args)...);
		}
	};
}