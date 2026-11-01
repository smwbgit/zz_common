#pragma once

#include <windows.h>
#include <utils/ManagedHandle.hpp>

namespace utils::windows::ui::gdi::dc
{
	class Handle : public utils::ManagedHandle<HDC>
	{
	public:
		Handle(HDC hdc, HWND hwnd)
			: utils::ManagedHandle<HDC>{hdc}
		{
			m_close_handler = [hwnd](HDC hdc)
			{
				utils_assert(1 == ::ReleaseDC(hwnd, hdc));
			};
		}
		explicit Handle(HDC hdc)
			: utils::ManagedHandle<HDC>{hdc}
		{
			m_close_handler = [](HDC hdc)
			{
				utils_assert(::DeleteDC(hdc));
			};
		}
	};
}