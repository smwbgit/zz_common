#pragma once

#include <filesystem>
#include <utils/assert.hpp>

namespace utils::real_path::directory_iterator
{
	//目录遍历器
	//note: 非递归
	//todo: 变为可以支持递归的。通过添加参数
	class DirectoryIterator
	{
	public:
		//要遍历的目录
		std::u8string m_dirname;
		//遇到一个文件时的回调函数
		std::function<void(const std::u8string & filename)> m_meet_one_regular_file;

	public:
		//开始遍历
		void start()
		{
			//简单检查
			utils_assert(!m_dirname.empty());
			utils_assert(m_meet_one_regular_file);

			//准备数据: 错误码
			std::error_code error_code{};

			//标准遍历器
			std::filesystem::directory_iterator directory_iterator{ m_dirname, error_code };

			//简单检查: 打开目录成功
			utils_assert(!error_code);

			//循环
			while(1)
			{
				//没有文件要遍历
				if(directory_iterator == std::filesystem::directory_iterator{})
				{
					//不再继续
					return;
				}

				//获取当前文件
				const std::filesystem::directory_entry & entry = *directory_iterator;

				//获取文件信息
				const auto status = entry.status(error_code);

				//简单检查: 获取成功
				utils_assert(!error_code);

				//如果为普通文件
				if(std::filesystem::is_regular_file(status))
				{
					//文件名
					const auto filename = entry.path().u8string();

					//发起回调
					m_meet_one_regular_file(filename);
				}

				//移动到目录中的下一项
				directory_iterator.increment(error_code);

				//简单检查: 移动成功
				utils_assert(!error_code);
			}
		}
	};

#if 0
		class with_these_extension
	{
	public:
		//ext_regex: 扩展名的正则表达式
		with_these_extension(const std::string & ext_regex)	// 比如 "\\.X[0-9]+"
			: m_ext_regex {ext_regex}
		{
		}
		with_these_extension(const with_these_extension &) = default;
		with_these_extension(with_these_extension &&) = default;

		bool is_match(const std::filesystem::path & path) const
		{
			return std::regex_match(path.extension().string(), m_ext_regex);
		}
	private:
		std::regex m_ext_regex;
	};
	//找出扩展名符合正则表达式的那些文件
	inline void directory_iterator(const std::filesystem::path & dirname, const with_these_extension & extension_arg,
		std::function<void(const std::string &)> meet_one_file)
	{
		namespace fs = std::filesystem;
		try
		{
			for(auto & p: fs::directory_iterator(dirname))
			{
				if (false == extension_arg.is_match(p.path()))
					continue;

				meet_one_file(p.path().string());
			}
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}
			
	class with_this_extension
	{
	public:
		with_this_extension() = default;
		with_this_extension(const std::string & ext)	// 比如 ".out"
			: m_ext {ext}
		{
		}
		with_this_extension(const with_this_extension &) = default;
		with_this_extension(with_this_extension &&) = default;

		const std::string & get() const	//为了加速，直接返回引用类型
		{
			return m_ext;
		}
	private:
		std::string m_ext;
	};
	//找出是这个扩展名的那些文件
	inline void directory_iterator(const std::filesystem::path & dirname, const with_this_extension & extension_arg, std::function<void(const std::string &)> meet_one_file)
	{
		namespace fs = std::filesystem;
		try
		{
			for(auto & p: fs::directory_iterator(dirname))
			{
				if (p.path().extension() != extension_arg.get())
					continue;

				meet_one_file(p.path().string());
			}
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}

#endif
}