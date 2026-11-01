#pragma once

#include <windows.h>
#include <utils/windows/ui/gdi/object/Object.hpp>
#include <utils/integer_type.hpp>
#include <utils/basic/store_this_integer.hpp>
#include <utils/basic/align.hpp>

namespace utils::windows::ui::gdi::object::bitmap
{
	// 位图
	using Bitmap = utils::windows::ui::gdi::object::Object<HBITMAP>;

	// 获取位图的对齐大小
	inline u8 align_size(const u8 size)
	{
		//按 4 字节对齐
		const auto size_2 = utils::basic::align(size, 4);

		//给出
		return size_2;
	}
}