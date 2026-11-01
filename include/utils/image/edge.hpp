#pragma once

#include <utils/windows/gdiplus.hpp>
#include <utils/text/string.hpp>

//边缘
namespace utils::image::edge
{
	//提取边缘
	//bmp_filename: 输入图片
	//result_dirname: 输出目录
	//point_sizes: 标记的点数，每个生成一个图片
	inline void make(
		const std::filesystem::path & bmp_filename,
		const std::filesystem::path & result_dirname,
		const std::vector<uint64_t> & point_sizes)
	{
		utils::windows::gdiplus::Bitmap bmp{bmp_filename};
	
		struct s_point
		{
			utils::Index2D pos;
			int64_t diff{};
		};
		std::vector<s_point> points;
		auto get_diff = [&bmp](utils::Index2D pos_0, utils::Index2D pos_1) -> int64_t
		{
			if (pos_1.i >= bmp.size().i) return 0;
			if (pos_1.j >= bmp.size().j) return 0;

			auto col0 = bmp.get_pixel(pos_0);
			auto col1 = bmp.get_pixel(pos_1);

			auto r = (int)GetRValue(col0)-GetRValue(col1);
			auto g = (int)GetGValue(col0)-GetGValue(col1);
			auto b = (int)GetBValue(col0)-GetBValue(col1);

			return r*r+g*g+b*b;
		};
		for(const auto & pos : bmp.size())
		{
			auto diff = (std::max)(
				get_diff(pos, pos.return_shift(1,0)),
				get_diff(pos, pos.return_shift(0,1)));
			points.push_back({pos, diff});
		}

		bmp.unlock();
		std::sort(points.begin(), points.end(), [](const auto & a, const auto & b)
		{
			return a.diff > b.diff;
		});
		auto make = [&points, &bmp, &result_dirname](int64_t n)
		{
			auto bmp2 = bmp.copy();
			for(int i = 0; i < n && i < bmp.size().size_1D(); i++)
				bmp2.set_pixel(points[i].pos, RGB(255,0,0));
			bmp2.save(result_dirname / utils::text::string().format_sprintf(uR"(diff_%lld.bmp)", n).m_text);
		};
		for(const auto n : point_sizes)
		{
			make(n);
		}
	}
}