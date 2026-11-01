#pragma once


#include "utils/computer_basic/store_this_integer.hpp"


namespace utils::ui::math::page
{
	template<typename T>
	void get_middle_line_pos(T& middle_pos, uint64_t line_n)
	{
		uint64_t _middle_pos = line_n == 0 ? 0 : (line_n - 1) / 2;
		utils::computer_basic_function::store_this_integer(middle_pos, _middle_pos);
	}
}