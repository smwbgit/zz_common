#pragma once

#include <utils/ui/layout/layout_system/common.hpp>

//横竖变换
namespace utils::ui::layout::layout_system::compute::change_orientation
{
	inline void change_inplace(utils::Index2D & value)
	{
		std::swap(value.i, value.j);
	}
	inline void change_inplace(Requirement & requirement)
	{
		change_inplace(requirement.min_size);
		change_inplace(requirement.hint_size);
		change_inplace(requirement.max_size);
	}
	inline void change_inplace(AllRequirement & all_requirement)
	{
		for(auto & requirement : all_requirement)
			change_inplace(requirement);
	}
	inline void change_inplace(bool & value)
	{
		value = !value;
	}
}