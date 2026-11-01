#pragma once

#include <string>
#include <functional>
#include <regex>
#include <unordered_set>
#include <set>
#include <utils/standard_library_support/filesystem.hpp>
#include <utils/throw.hpp>
#include <utils/assert.hpp>
#include <utils/uuid.hpp>
#include <utils/path/real/get_information.hpp>

#ifdef utils_on_windows
#include <windows.h>
#endif

namespace utils::real_path
{
#if 0	//请改用 utils::path::real::get_information::single::is_exist
	//目录是否存在
	inline bool is_directory_exist(const std::string & dirname)
	{
		namespace fs = std::filesystem;

		//避免抛异常
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
	//文件是否存在
	inline bool is_file_exist(const std::filesystem::path & filename)
	{
		namespace fs = std::filesystem;

		//避免抛异常
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
#endif

	//删除文件
	//is_allow_not_exist: 是否允许不存在
	inline void delete_file(const std::filesystem::path & filename, bool is_allow_not_exist = true)
	{
		if (!is_allow_not_exist)
		{
			//简单检查
			utils_assert(utils::path::real::get_information::single::is_exist::
				is_file_exist(filename));
		}

		utils_assert(DeleteFileW(filename.wstring().c_str()));
	}
	//移动文件
	//is_allow_overwrite: 是否允许覆盖
	inline void move(const std::filesystem::path & from, const std::filesystem::path & to, bool is_allow_overwrite = false)
	{
		if (is_allow_overwrite)
		{
			if (utils::path::real::get_information::single::is_exist::
				is_file_exist(to))
			{
				delete_file(to);
			}
		}
		utils_assert(MoveFileW(from.wstring().c_str(), to.wstring().c_str()));
	}

	//获得exe所在目录
	inline std::filesystem::path get_exe_directory()
	{
#ifdef utils_on_windows
		char Drive[_MAX_DRIVE]{};
		char Dir[MAX_PATH]{};
		char ExePath[MAX_PATH]{};

		auto re = GetModuleFileNameA(NULL, ExePath, MAX_PATH);	//todo 看看在dll中，第一个参数应该怎么写

		//简单检查
		utils_assert(re != 0 && re != MAX_PATH);

		_splitpath(ExePath, Drive, Dir, NULL, NULL);
		return {std::string() + Drive + Dir};
#else
#	error
#endif
	}

	//获得系统临时目录
	inline std::filesystem::path get_system_temp_dir()
	{
#ifdef utils_on_windows
		//获取系统temp目录
		char strTmpPath[MAX_PATH]{};
		auto re = GetTempPathA(sizeof(strTmpPath), strTmpPath);

		//简单检查
		utils_assert(re != 0 && re <= MAX_PATH);

		return strTmpPath;
#else
#	error
#endif
	}

	//如果指定文件的所在目录不存在则创建
	inline void create_its_directory_if_not_exist(const std::filesystem::path & filename)
	{
		namespace fs = std::filesystem;

		try
		{
			fs::create_directories(fs::path{filename}.parent_path());	//如果已经存在，返回false，不存在返回true
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}


	//在指定目录里创建一个新文件夹
	//parent_dirname：指定目录
	//dirname：指定目录 + '/' + 目录名
	inline void create_temp_dir(std::string & dirname, const std::string & parent_dirname)
	{
		std::string uuid;
		utils::uuid_function::get_string_with_dash(uuid);
		dirname = parent_dirname + '/' + uuid;

		namespace fs = std::filesystem;
		try
		{
			fs::create_directory(dirname);
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}

	//获取一个临时文件名。如果没有指定目录，则目录为系统的临时文件目录
	//dirname：指定目录
	//return：指定目录 + '/' + 文件名
	inline std::string get_temp_filename(const std::string & dirname = "")
	{
		//如果没有指定目录，则为系统的临时文件目录
		std::string _dirname = dirname == "" ? get_system_temp_dir().string() : dirname;

		std::string uuid;
		utils::uuid_function::get_string_with_dash(uuid);
		std::string filename = _dirname + '/' + uuid;

		//简单检查
		utils_assert(utils::path::real::get_information::single::is_exist::
			is_file_exist(filename) == false);

		return filename;
	}

	//如果指定目录不存在则创建
	inline void create_directory_if_not_exist(const std::filesystem::path & dirname)
	{
		namespace fs = std::filesystem;

		try
		{
			fs::create_directories(dirname);	//如果已经存在，返回false，不存在返回true
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}
	//创建一个还不存在的目录
	inline void create_not_exist_directory(const std::string & dirname)
	{
		namespace fs = std::filesystem;

		try
		{
			auto re = fs::create_directories(dirname);	//如果已经存在，返回false，不存在返回true
			utils_assert(re);
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}

	//从相对路径获得绝对路径
	inline void get_absolute(std::string & absolute_filename, const std::string & relative_filename)
	{
		namespace fs = std::filesystem;

		try
		{
			fs::path p{relative_filename};
			absolute_filename = fs::absolute(p).string();
		}
		catch(fs::filesystem_error)
		{
			utils_throw();
		}
	}
	//特化
	inline std::string get_absolute(const std::string & relative_filename)
	{
		std::string absolute_filename;
		get_absolute(absolute_filename, relative_filename);
		return absolute_filename;
	}

	//获得文件夹里有哪些扩展名
	//note: 不遍历子目录
	template<typename set_T>
	inline void get_exts(set_T & exts, const std::string & dirname)
	{
		static_assert(
			std::is_same_v<set_T, std::unordered_set<std::string>> || 
			std::is_same_v<set_T, std::set<std::string>>
		);

		namespace fs = std::filesystem;

		exts.clear();
		try
		{
			for (auto & p : fs::directory_iterator(dirname))
			{
				if (false == p.path().has_extension())
					continue;

				exts.insert(p.path().extension().string());
			}
		}
		catch(fs::filesystem_error)
		{
			utils_throw();
		}
	}


	//文件如果存在则删除
	inline void remove_file_if_exist(const std::string & filename)
	{
		namespace fs = std::filesystem;

		try
		{
			fs::remove(filename);
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}

	
	//安全删除某个文件
	//note：遍历目录下的所有文件，和传来的文件名比对，有则删
	//      防止文件名里有..之类的导致的误删
	//dirname：文件所在目录	e.g.	e:/abc/d
	//filename：文件名		e.g.	1.txt
	inline void safe_remove_file_if_exist(const std::string & dirname, const std::string & filename)
	{
		//目录不存在则返回
		if (false == utils::path::real::get_information::single::is_exist::
			is_directory_exist(dirname))
		{
			return;
		}

		//遍历目录
		namespace fs = std::filesystem;
		try
		{
			for(auto & p: fs::directory_iterator(dirname))
			{
				if (p.path().filename() != filename)
					continue;

				fs::remove(p.path());
				break;
			}
		}
		catch(fs::filesystem_error &)
		{
			utils_throw();
		}
	}
#if 0	//note: 请改为使用 utils::path::real::get_information::global::get_all_filename::by_directory
	//获得目录里所有文件名
	//is_recursive: 是否递归
	//is_want: 由外部判断，此文件名是否要
	inline std::vector<std::u8string> get_filenames_in_directory(const std::u8string & dirname,
		bool is_recursive, std::function<bool(const std::u8string & filename)> is_want = {})
	{
	}
#endif
}