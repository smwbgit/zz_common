#pragma once

#include <windows.h>
#include <utils/windows/ui/window/s_hwnd.hpp>
#include <utils/windows/ui/gdi/dc/MemoryDC.hpp>

//开始绘制。响应WM_PAINT消息
namespace utils::windows::ui::gdi::begin_paint
{
	//简单模式
	inline void simple(HWND hwnd, std::function<void(HDC hdc)> paint)
	{
		//开始绘制
		PAINTSTRUCT ps{};
		HDC hdc = BeginPaint(hwnd, &ps);

		//绘制
		paint(hdc);

		//结束绘制
        EndPaint(hwnd, &ps);
	}
	//准备一个内存dc，在上面绘制
	inline void with_memdc(HWND hwnd, std::function<void(HDC hdc)> paint)
	{
		return simple(hwnd, [hwnd, paint](HDC hdc)
		{
			//获取客户区大小
			const auto size = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_client_size();

			//创建内存dc
			utils::windows::ui::gdi::dc::MemoryDC memdc{hdc, size};

			//绘制
			paint(memdc.get_hdc());

			//拷贝
			BitBlt(hdc, 0, 0, size.i, size.j, memdc.get_hdc(), 0, 0, SRCCOPY);
		});
	}
}