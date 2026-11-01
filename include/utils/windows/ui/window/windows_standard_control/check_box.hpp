#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/integer_type.hpp>

namespace utils::windows::ui::window::windows_standard_control::check_box
{
	inline bool is_checked(HWND hwnd)
	{
		const u8 checked_flag = SendMessageW(hwnd, BM_GETCHECK, 0, 0);

		//简单检查
		utils_assert(checked_flag == BST_CHECKED || checked_flag == BST_UNCHECKED);

		return checked_flag == BST_CHECKED;
	}
	inline void set_checked(HWND hwnd, bool is_checked)
	{
		const u8 checked_flag = is_checked ? BST_CHECKED : BST_UNCHECKED;

		utils_assert(0 == SendMessageW(hwnd, BM_SETCHECK, checked_flag, 0));
	}
}