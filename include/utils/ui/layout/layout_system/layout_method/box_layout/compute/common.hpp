#pragma once

#include <utils/ui/layout/layout_system/layout_method/box_layout/common.hpp>

namespace utils::ui::layout::layout_system::layout_method::box_layout::compute
{
	//函数参数
	struct calculate_requirement__params
	{
		utils::pointer::not_null_for_user<const AllRequirement> all_requirement;
		utils::pointer::not_null_for_user<const Setting> setting;
	};
	//函数参数
	struct calculate_layout__params
	{
		utils::Index2D window_size;
		utils::pointer::not_null_for_user<const Requirement> requirement;
		utils::pointer::not_null_for_user<const AllRequirement> all_requirement;
		utils::pointer::not_null_for_user<const Setting> setting;
	};
}