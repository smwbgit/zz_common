#pragma once

#include <windows.h>
#include <utils/Index2D.hpp>
#include <utils/image/Image2D.hpp>

namespace utils::windows::window::paint::dc
{
	//设备无关位图
	class DIBitmap
	{
	private:
		HBITMAP m_hbitmap{};
		utils::Index2D m_size;
	public:
		DIBitmap(const utils::image::Image2D & image)
		{
			//ai生成后修改

			// 填充BITMAPINFOHEADER结构
			BITMAPINFOHEADER bi{};
			bi.biSize = sizeof(BITMAPINFOHEADER);
			bi.biWidth = image.get_size().i;
			bi.biHeight = -image.get_size().j; // 对于位图，高度通常是负的
			bi.biPlanes = 1;
			bi.biBitCount = 24;
			bi.biCompression = BI_RGB;
			bi.biSizeImage = 0;

			// 调用CreateDIBSection来创建HBITMAP
			m_hbitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS, NULL, NULL, 0);
			utils_assert(m_hbitmap);

			// 现在你需要获取位图的像素数据指针
			BITMAP bitmap{};
			GetObjectA(m_hbitmap, sizeof(bitmap), &bitmap);
			BYTE* dibBits = (BYTE*)bitmap.bmBits;
			utils_assert(dibBits);
			
			//简单检查
			utils_assert(sizeof(utils::image::pixel::Pixel) == 3);
			
			//refer to: BITMAPINFOHEADER 文档
			uint64_t stride = ((((image.get_size().i * 24) + 31) & ~31) >> 3);

			for(int64_t j = 0; j < image.get_size().j; j++)
			{
				memcpy(
					dibBits + stride * j,
					image.get_data() + image.get_size().i * j * 3,
					image.get_size().i * 3);
			}
			//RGB to BGR
			for(const auto pos : image.get_size())
			{
				auto pixel = dibBits + stride * pos.j + pos.i * 3;
				std::swap(pixel[0], pixel[2]);
			}

			//保存
			m_size = image.get_size();
		}
		~DIBitmap()
		{
			utils_assert_abort(DeleteObject(m_hbitmap));
		}
		void draw_to(HDC hdc)
		{
			HDC hdc_src = CreateCompatibleDC(hdc);
			utils_assert(hdc_src);

			HBITMAP hbitmap_old = (HBITMAP)SelectObject(hdc_src, m_hbitmap);
			utils_assert(hbitmap_old);

			BitBlt(hdc, 0, 0, m_size.i, m_size.j, hdc_src, 0, 0, SRCCOPY);

			SelectObject(hdc_src, hbitmap_old);
			utils_assert(DeleteDC(hdc_src));
		}
	};
}