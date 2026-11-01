#pragma once

#include <functional>
#include <utils/standard_library_support/filesystem.hpp>
#include <utils/u8string.hpp>

#ifdef utils_on_windows
#include <windows.h>
#endif

namespace utils::path::real
{
	//内部使用，方便后续书写
	namespace fs = std::filesystem;
}

#pragma region global
//获得目录里所有文件名
namespace utils::path::real::get_information::global::get_all_filename
{
	//获得目录里所有文件名
	//is_recursive: 是否递归
	//is_want: 由外部判断，是否留下此文件名
	inline std::vector<std::filesystem::path> by_directory(const std::filesystem::path & dirname,
		bool is_recursive, std::function<bool(const std::filesystem::path & filename)> is_want = {})
	{
		try
		{
			std::error_code ec{};

			//简单检查
			utils_assert(fs::exists(dirname, ec)); utils_assert(!ec);
			utils_assert(fs::is_directory(dirname, ec)); utils_assert(!ec);

			std::vector<std::filesystem::path> all_filename;
			auto iterator = fs::recursive_directory_iterator(dirname, ec); utils_assert(!ec);
		
			for (auto iter = std::filesystem::begin(iterator);
				iter != std::filesystem::end(iterator);)
			{
				const auto & one = *iter;

				if (one.is_regular_file())
				{
					const auto filename = one.path();

					if(!is_want || is_want(filename))
						all_filename.push_back(filename);
				}

				//不递归
				if(!is_recursive)
					iterator.disable_recursion_pending();

				iter.increment(ec); utils_assert(!ec);
			}
			return all_filename;
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}
}
//找出，指定文件夹中，修改时间，最新的那个文件
namespace utils::path::real::get_information::global::get_last_write_file
{
	//note: 不迭代，也就是不考虑子文件夹
	inline std::optional<std::u8string> by_directory(const std::u8string & dirname)
	{
		//获取所有的文件名
		const auto filenames = utils::path::real::get_information::global::
			get_all_filename::by_directory(dirname, false);

		//准备数据
		std::optional<std::u8string> return_value;

		//准备数据
		auto latest_time = (std::filesystem::file_time_type::min)();

		//对于每一个文件
		for(const auto & filename : filenames)
		{
			//准备数据
			std::error_code ec{};

			//查询
			auto t = std::filesystem::last_write_time(filename, ec);

			//简单检查
			utils_assert(!ec);

			//修改时间更近
			if (t > latest_time)
			{
				//更新状态
				latest_time = t;

				//保存
				return_value = filename.u8string();
			}
		}

		//给出
		return return_value;
	}
}
#pragma endregion

#pragma region single
//是否存在
namespace utils::path::real::get_information::single::is_exist
{
	//名称是否存在
	inline bool is_name_exist(const std::filesystem::path & filename)
	{
		//准备数据
		std::error_code ec;

		//查询文件信息
		const auto status = fs::status(filename, ec);

		//名称不存在
		if(ec == std::errc::no_such_file_or_directory)
			return false;

		//权限不足
		if(ec == std::errc::permission_denied)
			utils_throw();

		//其他问题
		if(ec)
			utils_throw();

		//名称存在
		return true;
	}
	//文件是否存在
	inline bool is_file_exist(const std::filesystem::path & filename)
	{
		//准备数据
		std::error_code ec;

		//查询文件信息
		const auto status = fs::status(filename, ec);

		//文件不存在
		if(ec == std::errc::no_such_file_or_directory)
			return false;

		//权限不足
		if(ec == std::errc::permission_denied)
			utils_throw();

		//其他问题
		if(ec)
			utils_throw();

		//是否为普通文件
		const auto is_file = fs::is_regular_file(status);

		//给出
		return is_file;
	}
	//目录是否存在
	inline bool is_directory_exist(const std::filesystem::path & dirname)
	{
		//准备数据
		std::error_code ec;

		//查询文件信息
		const auto status = fs::status(dirname, ec);

		//文件不存在
		if(ec == std::errc::no_such_file_or_directory)
			return false;

		//权限不足
		if(ec == std::errc::permission_denied)
			utils_throw();

		//其他问题
		if(ec)
			utils_throw();

		//是否为目录
		const auto is_directory = fs::is_directory(status);

		//给出
		return is_directory;
	}
}
#pragma endregion