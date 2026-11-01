#pragma once

#include <windows.h>
#include <utils/image/Image2D.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>
#include <utils/windows/ui/gdi/object/bitmap.hpp>

// 和Image2D相关
namespace utils::windows::ui::gdi::image2d
{
	// 位图变为图像
	inline void to_image2d(utils::image::Image2D & image, HDC hdc, HBITMAP hBitmap,
		utils::Index2D hdc_size, const utils::Range2D & want_rect,
		utils::image::pixel::PixelFormat want_pixel_format = utils::image::pixel::PixelFormat::RGB)
	{
		// 简单检查
		utils_assert(hdc_size.i > 0);
		utils_assert(hdc_size.j > 0);
		utils_assert(want_rect.position.i >= 0);
		utils_assert(want_rect.position.j >= 0);
		utils_assert(want_rect.size.i > 0);
		utils_assert(want_rect.size.j > 0);
		utils_assert(want_rect.is_in({{},hdc_size}));

		// 便利声明
		using s_hdc = utils::windows::ui::gdi::dc::s_hdc;

		// 创建内存DC
		auto mem_dc = s_hdc(hdc).create_compatible_dc();

		// 选入位图
		auto auto_clean = s_hdc(hdc).select(hBitmap);

		// 准备数据
		BITMAPINFOHEADER bi = {
			sizeof(BITMAPINFOHEADER),
			utils::basic::store_in<LONG>(hdc_size.i),
			- utils::basic::store_in<LONG>(hdc_size.j), // 负高度表示从上到下存储
			1,
			24,
			BI_RGB
		};

		// 第一次拷贝时,每行的大小
		const auto line_size_in_byte = utils::windows::ui::gdi::object::bitmap::align_size(hdc_size.i * 3);

		// 第一次拷贝时,总大小
		const auto size_in_byte = line_size_in_byte * want_rect.size.j;

		// 准备数据
		std::vector<u1> buffer(size_in_byte);

		// 第一次拷贝
		utils_assert(GetDIBits(mem_dc, hBitmap, want_rect.position.j, want_rect.size.j, buffer.data(),
			(BITMAPINFO*)&bi, DIB_RGB_COLORS));

		// 第二次拷贝时,目标数据区
		image.set_size(want_rect.size);
		auto dst = image.get_data();

		// 第二次拷贝时,源数据
		auto src = buffer.data();
		
		// 源位置,挪动到第一行上,要拷贝的像素上
		src += 3 * want_rect.position.i;

		// 复制
		for (s8 j = 0; j < want_rect.size.j; j++)
		{
			// 复制一行
			memmove(dst, src, 3 * want_rect.size.i);

			// 便利声明
			using PixelFormat = utils::image::pixel::PixelFormat;
			using Pixel = utils::image::pixel::Pixel;

			// 看看用户想要什么像素格式
			switch(want_pixel_format)
			{
			case PixelFormat::RGB:
				// 对于每个像素, 交换 B 和 R
				utils::image::pixel::change_pixel_format::in_some_pixels(
					(Pixel*)dst, want_rect.size.i, PixelFormat::BGR, PixelFormat::RGB);
				// 完成
				break;
			case PixelFormat::BGR:
				// 无需额外动作
				break;
			default:
				// 报错
				utils_throw();
			}

			// 移动到下一行
			src += line_size_in_byte;
			dst += want_rect.size.i * 3;
		}

		// 保存像素格式
		image.m_pixel_format = want_pixel_format;
	}

	// 图像变为位图
	inline auto from_image2d(const utils::image::Image2D & image, HDC hdc)
		-> utils::windows::ui::gdi::object::bitmap::Bitmap
	{
		// 准备数据
		BITMAPINFO bi = {
			sizeof(BITMAPINFOHEADER),
			utils::basic::store_in<LONG>(image.get_size().i),
			- utils::basic::store_in<LONG>(image.get_size().j), // 负高度表示从上到下存储
			1,
			24,
			BI_RGB
		};

		// 准备数据
		u1* bitmap_data{};
		HBITMAP hbitmap = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&bitmap_data, nullptr, 0);

		// 简单检查
		utils_assert(hbitmap);
		utils_assert(bitmap_data);

		// 添加自动清理功能
		utils::windows::ui::gdi::object::bitmap::Bitmap bitmap{ hbitmap };
		
		// 复制时: 源位置
		auto from = image.get_data();

		// 复制时: 目标位置
		auto to = bitmap_data;

		// 复制时: 一行的字节大小
		const auto size = image.get_size().i * 3;

		// 复制时: 位图的一行的字节大小
		const auto size_2 = utils::windows::ui::gdi::object::bitmap::align_size(size);

		// 对于每一行
		for(const auto row : utils::iterator_index(image.get_size().j))
		{
			// 复制一行
			memmove(to, from, size);

			// 便利声明
			using Pixel = utils::image::pixel::Pixel;
			using PixelFormat = utils::image::pixel::PixelFormat;

			// 检查像素格式
			switch(image.m_pixel_format)
			{
			case PixelFormat::RGB:
				// 对于每个像素, 交换 B 和 R
				utils::image::pixel::change_pixel_format::in_some_pixels(
					(Pixel*)to, image.get_size().i, PixelFormat::RGB, PixelFormat::BGR);
				// 完成
				break;
			case PixelFormat::BGR:
				// 无需任何动作
				break;
			default:
				// 报错
				utils_throw();
			}

			// 移动到下一行
			from += size;
			to += size_2;
		}

		//给出
		return std::move(bitmap);
	}
}