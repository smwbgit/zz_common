#pragma once

#include <windows.h>
#include <utils/Index2D.hpp>

namespace utils::windows::ui::gdi::dc
{
	//内存dc
	class MemoryDC
	{
	private:
		HDC m_hdc{};
		HBITMAP m_hbitmap{};
		HBITMAP m_hbitmap_old{};

	public:
		MemoryDC(HDC hdc, utils::Index2D size)
		{
			//简单检查
			utils_assert(hdc);
			
			//创建dc
			m_hdc = CreateCompatibleDC(hdc);
			utils_assert(m_hdc);

			//创建位图
			m_hbitmap = CreateCompatibleBitmap(hdc /*不能是m_hdc。不然显示时会变成黑白照片*/,
				size.i, size.j);
			utils_assert(m_hbitmap);

			//使用位图
			m_hbitmap_old = (HBITMAP)SelectObject(m_hdc, m_hbitmap);
			utils_assert(m_hbitmap_old);
		}
		//特化
		MemoryDC(HWND hwnd, utils::Index2D size)
		{
			//获取窗口dc
			auto hdc = GetDC(hwnd);
			utils_assert(hdc);
			
			//转发
			MemoryDC(hdc, size);

			//释放dc
			utils_assert(ReleaseDC(hwnd, hdc));
		}
		~MemoryDC()
		{
			//不再使用位图
			SelectObject(m_hdc, m_hbitmap_old);

			//删除位图
			utils_assert_abort(DeleteObject(m_hbitmap));

			//删除dc
			utils_assert_abort(DeleteDC(m_hdc));
		}
		MemoryDC(const MemoryDC&) = delete;
		MemoryDC(MemoryDC&&) = delete;
		MemoryDC & operator=(const MemoryDC&) = delete;
		MemoryDC & operator=(MemoryDC&&) = delete;

	public:
		//获取句柄
		HDC get_hdc() const
		{
			return m_hdc;
		}
		//获取位图句柄
		HBITMAP get_hbitmap() const
		{
			return m_hbitmap;
		}
	public:
		operator HDC() const
		{
			//转发
			return get_hdc();
		}
	};
}