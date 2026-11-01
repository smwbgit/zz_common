#pragma once

namespace utils::windows::mfc::message
{
	//由于有时WM_USER会被别的什么使用，所以使用时我跳过了100个
	inline constexpr UINT _WM_USER = WM_USER + 100;
}
