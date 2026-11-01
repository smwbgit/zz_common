#pragma once

#include <utils/filesystem.hpp>
#include <utils/text/layout.hpp>
#include <utils/text/friendly/data_size.hpp>

namespace utils::real_path::small_tool
{
	namespace inner::get_big_file_list
	{
		struct s_directory_info
		{
			uint64_t size;
			std::u8string text;
		};

		inline std::u8string make_one_line(const std::filesystem::path & path,
			uint64_t size, const std::u8string & type)
		{
			const auto text_size = utils::text::friendly::data_size(size);
			return utils::text::layout::right_align(text_size, 8) + u8"  "
				+ type + u8" "
				+ path.u8string() + u8"  "
				+ text_size + u8"\n";
		}
		inline s_directory_info get_directory_info(const std::filesystem::path & path, const uint64_t base_size)
		{
			//简单检查
			utils_assert(utils::filesystem::is_directory(path));
			
			struct s_node_info
			{
				uint64_t size;
				std::u8string text;
			};
			std::vector<s_node_info> infos;
			try
			{
				std::filesystem::directory_iterator it{ path, std::filesystem::directory_options::skip_permission_denied };
				for (const auto & p : it)
				{
					try
					{
						if (std::filesystem::is_regular_file(p.path()))
						{
							const auto size = std::filesystem::file_size(p.path());	//此处故意使用std而不是utils里的,以便错误出现后被catch
							const auto text = make_one_line(p.path(), size, u8"- ");
							infos.push_back({ size, text });
						}
						else if (std::filesystem::is_directory(p.path()))
						{
							const auto directory_info = get_directory_info(p.path(), base_size);
							const auto & size = directory_info.size;
							const auto & text = directory_info.text;
							infos.push_back({ size, text });
						}
					}
					catch(std::filesystem::filesystem_error &)
					{
					}
				}
			}
			catch(std::filesystem::filesystem_error &)
			{
			}

			std::sort(infos.begin(), infos.end(), [](const auto & a, const auto & b)
			{
				return a.size < b.size;
			});

			s_directory_info directory_info{};
			for(const auto & one : infos)
			{
				directory_info.size += one.size;
				
				if (one.size < base_size)
					continue;

				directory_info.text += one.text;
			}
			directory_info.text += make_one_line(path, directory_info.size, u8"d ");
			return directory_info;
		}
	}
	//获取所有比较大的文件和文件夹的列表
	//base_size: 这个大小往上的才会列出来
	inline std::u8string get_big_file_list(const std::filesystem::path & path, const uint64_t base_size = 1024 * 1024 * 1024)
	{
		//简单检查
		utils_assert(utils::filesystem::is_directory(path));

		const auto & info = inner::get_big_file_list::get_directory_info(path, base_size);
		return info.text;
	}
}