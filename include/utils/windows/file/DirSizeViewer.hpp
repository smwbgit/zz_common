#pragma once

#include <utils/real_path.hpp>
#include <utils/text.hpp>
#include <utils/FileWriter.hpp>
#include <utils/FileReader.hpp>

namespace utils::windows::filesystem
{
	inline std::vector<std::u16string> get_all_drives()
	{
		auto len = GetLogicalDriveStringsW(0, 0);
		utils_assert(len > 0);	//简单检查

		std::u16string buffer;
		buffer.resize(len+1);

		auto re = GetLogicalDriveStringsW(len, (wchar_t*)buffer.c_str());
		utils_assert(re+1 == len);		//简单检查
		utils_assert(buffer[re] == 0);	//简单检查

		std::vector<std::u16string> drives;
		const char16_t* p = buffer.c_str();
		while(1)
		{
			if(*p == 0)
				break;

			std::u16string drive = p;
			if([&]()
			{
				//判断当前磁盘类型
				int nType = GetDriveTypeW((const wchar_t*)drive.c_str());

				return nType == DRIVE_FIXED; //硬盘
			}())
				drives.push_back(drive);
			p+=drive.size()+1;
		}

		utils_assert(!drives.empty());	//简单检查
		return drives;
	}
}

namespace utils::windows
{
#if 0
	参考程序，不一定最新：
		
void init()
{
	setlocale(LC_ALL, "chs");
}
void work6()
{
	utils::windows::DirSizeViewer viewer{g_filename};
	
	std::filesystem::path cur = L"d:\\ZZ\\";
	while(1)
	{
		const auto & info = viewer.get_dir_info(cur);

		printf("dir %S:\n", info.full_name.c_str());
		printf("文件夹:\n");
		for(const auto & one : utils::iterator_and_give_me_i(info.dirs))
		{
			printf("%lld\t%s\t%S\n", one.i, utils::text::friendly::size::normal(one.value.size).c_str(), one.value.name.c_str());
		}
		printf("文件:\n");
		for(const auto & one : info.files)
		{
			printf("%s\t%S\n", utils::text::friendly::size::normal(one.size).c_str(), one.name.c_str());
		}

		int64_t index{};
		scanf("%lld", &index);
		
		if(index == -1)
		{
			cur = info.get_parent_path();
		}
		else if (index >= 0 && (uint64_t)index < info.dirs.size())
		{
			cur = info.get_child_path(index);
		}
		else
			utils_throw();
	}
	return;
}

#endif

	//计算文件夹大小
	class DirSizeViewer
	{
	private:
		struct s_file
		{
			std::wstring name;
			uint64_t size{};
			std::chrono::file_clock::time_point last_modify{};
		public:
			template<bool is_saving, typename T>
			void serialize_support(T& file)
			{
				file & name;
				file & size;
		//		file & last_modify;
			}
		public:
			void scan(const std::filesystem::path & filename)
			{
				name = filename.filename().wstring();
				size = std::filesystem::file_size(filename);
			}
		};
		struct s_dir
		{
			std::wstring full_name;
			std::wstring name;
			uint64_t size{};
			std::chrono::file_clock::time_point last_modify{};
			std::vector<s_dir> dirs;
			std::vector<s_file> files;

		public:
			template<bool is_saving, typename T>
			void serialize_support(T& file)
			{
				file & full_name;
				file & name;
				file & size;
		//		file & last_modify;
				file & dirs;
				file & files;
			}

		public:
			std::filesystem::path get_child_path(uint64_t index) const
			{
				//简单检查
				utils_assert(index < dirs.size());

				return dirs[index].full_name;
			}
			std::filesystem::path get_parent_path() const
			{
				if(name.empty())
					return {};
				if(std::filesystem::path{full_name}.root_path() == full_name)
					return {};
				return std::filesystem::path{full_name}.parent_path();
			}
			void scan(const std::filesystem::path & dirname)
			{
				for(const auto & one : std::filesystem::directory_iterator(dirname, std::filesystem::directory_options::skip_permission_denied))
				{
					try
					{
						if (one.is_directory())
						{
							s_dir dir;
							dir.scan(one.path());
							dirs.push_back(std::move(dir));
						}
						else if(one.is_regular_file())
						{
							s_file file;
							file.scan(one.path());
							files.push_back(std::move(file));
						}
						else
						{
							utils_throw();
						}
					}
					catch(std::filesystem::filesystem_error &)
					{
					}
				}

				full_name = dirname.wstring();

				size = 0;
				for(const auto & one : dirs)  size+=one.size;
				for(const auto & one : files) size+=one.size;

				if(dirname.root_path() == dirname)
					name = dirname.root_name().wstring();
				else
					name = dirname.filename().wstring();

				std::sort(dirs.begin(), dirs.end(), [](const auto & a, const auto & b) { return a.size > b.size; });
				std::sort(files.begin(), files.end(), [](const auto & a, const auto & b) { return a.size > b.size; });
			}
			void load_from_file(const std::filesystem::path & filename)
			{
				utils::FileReader input{filename};
				input.serializer & *this;
			}
			void save_to_file(const std::filesystem::path & filename)
			{
				utils::FileWriter output{filename};
				output.serializer & *this;
			}
		};
	private:
		s_dir m_dir;
	public:
		DirSizeViewer(const std::filesystem::path & save)
		{
			if(utils::real_path::is_file_exist(save))
			{
				m_dir.load_from_file(save);
			}
			else
			{
				//遍历每个盘符
				for(const auto & drive : utils::windows::filesystem::get_all_drives())
				{
					s_dir dir; dir.scan(drive); m_dir.dirs.push_back(std::move(dir));
				}

				m_dir.save_to_file(save);
			}
		}
		const s_dir & get_dir_info(const std::filesystem::path & path)
		{
			const s_dir* dir = &m_dir;
			for(const auto & one : path)
			{
				if(one == "\\") continue;
				if(one == "") continue;

				dir = [&]()
				{
					for(const auto & _one : dir->dirs)
					{
						if(utils::text::string{_one.name}.compare.iequal(one.wstring()))
							return &_one;
					}
					utils_throw();
				}();
			}
			return *dir;
		}
	};
}
