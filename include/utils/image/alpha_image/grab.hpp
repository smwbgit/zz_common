#pragma once

#include <windows.h>
#include <utils/assert.hpp>

//提取半透明图像
namespace utils::image::alpha_image::grab
{
	//提取：纯内存计算
	class Grabber
	{
	public:
		struct s_result
		{
			utils::windows::gdiplus::Bitmap bmp_min;
			utils::windows::gdiplus::Bitmap bmp_abs;
		};
	private:
		utils::Index2D m_size;
		std::vector<utils::math::s_maxmin_value<int64_t>> m_points;
	public:
		//计算一张图片
		void look_this(utils::windows::gdiplus::Bitmap & bmp)
		{
			//初始化
			if (m_points.empty())
			{
				m_size = bmp.size();
				m_points.resize(m_size.size_1D() * 3);
			}

			//简单检查
			utils_assert(bmp.size() == m_size);

			//计算
			for (const auto & pos : m_size)
			{
				auto color = bmp.get_pixel(pos);
				m_points[pos.index_1D(m_size) * 3 + 0].meet_one_value((int)GetRValue(color));
				m_points[pos.index_1D(m_size) * 3 + 1].meet_one_value((int)GetGValue(color));
				m_points[pos.index_1D(m_size) * 3 + 2].meet_one_value((int)GetBValue(color));
			}
		}
		//载入已计算好的
		void look_this_history(const s_result & history)
		{
			//简单检查
			utils_assert(history.bmp_min.size() == history.bmp_abs.size());

			//初始化
			if (m_points.empty())
			{
				m_size = history.bmp_min.size();
				m_points.resize(m_size.size_1D() * 3);
			}

			//简单检查
			utils_assert(history.bmp_min.size() == m_size);

			//载入
			for (const auto & pos : m_size)
			{
				auto color_min = history.bmp_min.get_pixel(pos);
				auto color_abs = history.bmp_abs.get_pixel(pos);

				m_points[pos.index_1D(m_size) * 3 + 0].meet_one_value((int)GetRValue(color_min));
				m_points[pos.index_1D(m_size) * 3 + 1].meet_one_value((int)GetGValue(color_min));
				m_points[pos.index_1D(m_size) * 3 + 2].meet_one_value((int)GetBValue(color_min));

				m_points[pos.index_1D(m_size) * 3 + 0].meet_one_value((int)GetRValue(color_min) + (int)GetRValue(color_abs));
				m_points[pos.index_1D(m_size) * 3 + 1].meet_one_value((int)GetGValue(color_min) + (int)GetGValue(color_abs));
				m_points[pos.index_1D(m_size) * 3 + 2].meet_one_value((int)GetBValue(color_min) + (int)GetBValue(color_abs));
			}
		}
		//获得计算结果
		auto get_result() -> s_result
		{
			//简单检查
			utils_assert(!m_points.empty());

			s_result result{{m_size},{m_size}};

			for (const auto & pos : m_size)
			{
				result.bmp_min.set_pixel(pos, RGB(
					m_points[pos.index_1D(m_size) * 3 + 0].get_min(),
					m_points[pos.index_1D(m_size) * 3 + 1].get_min(),
					m_points[pos.index_1D(m_size) * 3 + 2].get_min()));
				result.bmp_abs.set_pixel(pos, RGB(
					m_points[pos.index_1D(m_size) * 3 + 0].get_abs(),
					m_points[pos.index_1D(m_size) * 3 + 1].get_abs(),
					m_points[pos.index_1D(m_size) * 3 + 2].get_abs()));
			}
			return std::move(result);
		}
	};

	//提取：不仅计算，而且负责文件的载入和移动
	class FileManager
	{
	public:	//参数
		std::filesystem::path m_path_new_bmps;
		std::filesystem::path m_path_old_bmps;
		std::filesystem::path m_path_result;
	public:
		void make()
		{
			//简单检查
			utils_assert(!m_path_new_bmps.empty());
			utils_assert(!m_path_old_bmps.empty());
			utils_assert(!m_path_result.empty());

			std::vector<std::filesystem::path> filenames;
			utils::real_path::directory_iterator(m_path_new_bmps, utils::real_path::with_these_extension{R"(\.bmp|\.png)"}, [&](const auto & one)
			{
				filenames.push_back(one);
			});

			//计算新的图片
			Grabber grabber;
			for (const auto & filename : filenames)
			{
				utils::windows::gdiplus::Bitmap bmp{filename};
				grabber.look_this(bmp);
			}

			//载入已计算好的
			if (utils::real_path::is_file_exist(m_path_result / "min.bmp"))
			{
				//简单检查
				utils_assert(utils::real_path::is_file_exist(m_path_result / "abs.bmp"));

				Grabber::s_result result{{m_path_result / "min.bmp"},{m_path_result / "abs.bmp"}};
				grabber.look_this_history(result);
			}
			else
			{
				//简单检查
				utils_assert(!utils::real_path::is_file_exist(m_path_result / "abs.bmp"));
			}

			//保存
			auto result = grabber.get_result();
			result.bmp_min.save(m_path_result / "min.bmp");
			result.bmp_abs.save(m_path_result / "abs.bmp");

			//放入另一个文件夹
			utils::real_path::create_its_directory_if_not_exist(m_path_old_bmps/"1.bmp");
			for (const auto & filename : filenames)
				utils::real_path::move(filename, m_path_old_bmps / filename.filename());
		}
	};
}