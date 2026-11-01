#pragma once

#include <windows.h>
#include <utils/image/Image2D.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>
#include <utils/windows/ui/gdi/image2d.hpp>

namespace utils::windows::ui::gdi::dc
{
	//绘制图像
	inline void draw_image(HDC hdc, const utils::image::Image2D & image, utils::Index2D position)
	{
		// 图像大小为0
		if(image.get_size().is_exist_zero())
		{
			// 不再继续
			return;
		}
		
		// 便利声明
		using s_hdc = utils::windows::ui::gdi::dc::s_hdc;

		// 准备环境
		auto mem_dc = s_hdc(hdc).create_compatible_dc();

		// 图像变为位图
		auto bitmap = utils::windows::ui::gdi::image2d::from_image2d(image, mem_dc);

		// 准备环境
		bitmap.select_me(mem_dc);

		// 绘制
		s_hdc(hdc).copy_from(mem_dc, {}, position, image.get_size());

		// 清理环境
		bitmap.select_old(mem_dc);
	}
}