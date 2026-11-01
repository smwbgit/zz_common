#pragma once

#include <utils/image/Image2D.hpp>

namespace utils::image::compare::is_same
{
	//比较两张图片的指定部分是否相同
	inline bool two_part(
		const utils::image::Image2D & image_0, const utils::Index2D & image_0_start,
		const utils::image::Image2D & image_1, const utils::Index2D & image_1_start,
		const utils::Index2D & size)
	{
		//简单检查
		utils_assert(image_0_start.i >= 0);
		utils_assert(image_0_start.j >= 0);
		utils_assert(image_1_start.i >= 0);
		utils_assert(image_1_start.j >= 0);
		utils_assert((image_0_start + size).i <= image_0.get_size().i);
		utils_assert((image_0_start + size).j <= image_0.get_size().j);
		utils_assert((image_1_start + size).i <= image_1.get_size().i);
		utils_assert((image_1_start + size).j <= image_1.get_size().j);
		utils_assert(size.i >= 0);
		utils_assert(size.j >= 0);

		if (size.i == 0 || size.j == 0)
			return true;

		for (int64_t y = 0; y < size.j; y++)
		{
			auto index_0 = image_0.get_index_1D(image_0_start + utils::Index2D{0, y});
			auto index_1 = image_1.get_index_1D(image_1_start + utils::Index2D{0, y});
			if (memcmp(
				image_0.get_const_data() + index_0*3,
				image_1.get_const_data() + index_1*3,
				size.i * 3))
			{
				return false;
			}
		}
		return true;
	}
}