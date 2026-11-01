#pragma once

#include <utils/image/Image2D.hpp>

namespace utils::image::md5
{
	//方便使用
	using MD5Value = utils::data_encoding::md5::MD5Value;

	//获取一张图片的md5
	inline auto get(const utils::image::Image2D & bmp)
	{
		//所有像素
		const auto data = bmp.get_const_data();

		//像素数量
		const auto data_size = bmp.get_size().size_1D() * 3;

		//计算md5
		const auto md5 = utils::data_encoding::md5::encode(utils::data_view{data, data_size});

		//给出
		return md5;
	}
}