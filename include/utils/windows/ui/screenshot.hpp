#pragma once

#include <utils/windows/ui/gdi/image2d.hpp>
#include <utils/windows/ui/gdi/dc/MemoryDC.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>

// 屏幕截图
namespace utils::windows::ui::screenshot
{
	// 屏幕截图
	inline void get_image(utils::image::Image2D & image, utils::image::pixel::PixelFormat pixel_format = utils::image::pixel::PixelFormat::RGB)
	{
		// 获取屏幕DC
		const auto hdc_screen = ::GetDC(NULL);
		
		// 让它能自动释放
		utils::windows::ui::gdi::dc::Handle screen_dc{ hdc_screen, NULL };

		// 获取屏幕大小
		const auto screen_size = utils::windows::ui::screen::get_screen_size();

		// 因为后边的,GetDIBits 不能直接对 GetDC(0) 和 它的位图,进行调用,所以,此处需要先复制到内存DC里

		// 创建内存DC
		utils::windows::ui::gdi::dc::MemoryDC mem_dc{ hdc_screen, screen_size };

		// 便利引用
		using s_hdc = utils::windows::ui::gdi::dc::s_hdc;

		// 复制屏幕
		s_hdc(hdc_screen).copy_to(mem_dc, {}, {}, screen_size);

		// 把图像从 HBITMAP 转为 Image2D
		utils::windows::ui::gdi::image2d::to_image2d(
			image, mem_dc, mem_dc.get_hbitmap(), screen_size, {{}, screen_size}, pixel_format);
	}
	// 特化
	inline utils::image::Image2D get_image()
	{
		// 准备数据
		utils::image::Image2D image;

		// 转发
		get_image(image);

		// 给出
		return std::move(image);
	}
}