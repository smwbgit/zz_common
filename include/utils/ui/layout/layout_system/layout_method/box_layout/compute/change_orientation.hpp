#pragma once

#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/common.hpp>
#include <utils/ui/layout/layout_system/compute/change_orientation.hpp>

//横竖变换
namespace utils::ui::layout::layout_system::layout_method::box_layout::compute::change_orientation
{
	//向此命名空间中，添加更为通用的函数
	using utils::ui::layout::layout_system::compute::change_orientation::change_inplace;

	inline void change_inplace(Align & align)
	{
		std::swap(align.vertical, align.horizontal);
	}
	inline void change_inplace(OutsidePadding & outside_padding)
	{
		std::swap(outside_padding.left, outside_padding.top);
		std::swap(outside_padding.bottom, outside_padding.right);
	}
	inline void change_inplace(Setting & setting)
	{
		change_inplace(setting.is_vertical_layout);
		change_inplace(setting.align);
		//change_inplace(setting.inside_padding);	//不需要变换
		change_inplace(setting.outside_padding);
	}
	inline void change_inplace(utils::Range2D & rect)
	{
		change_inplace(rect.position);
		change_inplace(rect.size);
	}
#pragma region 此命名空间的核心函数
	//横竖变换: 计算需求
	inline void change_inplace(calculate_requirement__params & params)
	{
		//准备数据
		auto & all_requirement = const_cast<AllRequirement &>(*params.all_requirement);
		auto & setting = const_cast<Setting &>(*params.setting);

		//进行变换
		change_inplace(all_requirement);
		change_inplace(setting);
	}
	//横竖变换: 计算布局
	inline void change_inplace(calculate_layout__params & params)
	{
		//准备数据
		auto & window_size = params.window_size;
		auto & requirement = const_cast<Requirement &>(*params.requirement);
		auto & all_requirement = const_cast<AllRequirement &>(*params.all_requirement);
		auto & setting = const_cast<Setting &>(*params.setting);

		//进行变换
		change_inplace(window_size);
		change_inplace(requirement);
		change_inplace(all_requirement);
		change_inplace(setting);
	}
	//横竖变换: 布局结果
	inline void change_inplace(std::vector<utils::Range2D> & rects)
	{
		for(auto & rect : rects)
			change_inplace(rect);
	}
#pragma endregion
}