#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/Index2D.hpp>

namespace utils::windows::monitor
{
	//获得屏幕分辨率
	inline utils::Index2D get_screen_size()
	{
		HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
		MONITORINFOEXA miex{};
		miex.cbSize = sizeof(miex);
		utils_assert(GetMonitorInfoA(hMonitor, &miex));
		//miex.rcMonitor 是经过dpi缩放之后的
		DEVMODEA dm{};
		dm.dmSize = sizeof(dm);
		dm.dmDriverExtra = 0;
		utils_assert(EnumDisplaySettingsA(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm));
		return {dm.dmPelsWidth, dm.dmPelsHeight};
	}
	//特化
	inline void get_screen_size(_Out_ uint64_t& width, _Out_ uint64_t& height)
	{
		auto size = get_screen_size();
		width = size.i;
		height = size.j;
	}
}