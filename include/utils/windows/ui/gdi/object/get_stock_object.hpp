#pragma once

#include <windows.h>
#include <utils/assert.hpp>

//对GetStockObject()的简单包装
namespace utils::windows::ui::gdi::object::get_stock_object
{
	inline HBRUSH brush(_In_ int id)
	{
		auto brush = ::GetStockObject(id);
		utils_assert(brush);
		return (HBRUSH)brush;
	}
}