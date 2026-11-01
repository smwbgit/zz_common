#pragma once

#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/common.hpp>

//复制参数
namespace utils::ui::layout::layout_system::layout_method::box_layout::compute::copy_params
{
#pragma region 此命名空间的核心结构体
	//参数副本
	//note: 因为存在指针，为了深拷贝，对于被指针指向的对象，要建立对应的额外成员
	struct calculate_requirement__packed_params
	{
		//额外成员
		AllRequirement all_requirement;
		Setting setting;

		//参数副本
		calculate_requirement__params params;
	};
	//参数副本
	struct calculate_layout__packed_params
	{
		//额外成员
		Requirement requirement;
		AllRequirement all_requirement;
		Setting setting;

		//参数副本
		calculate_layout__params params;
	};
#pragma endregion
	
#pragma region 此命名空间的核心函数
	//获得参数副本: 计算需求
	inline auto get_packed_params(const calculate_requirement__params & params)
		-> calculate_requirement__packed_params
	{
		//建立空变量
		calculate_requirement__packed_params packed_params;

		//复制数据
		packed_params.all_requirement = *params.all_requirement;
		packed_params.setting = *params.setting;

		//建立指针
		packed_params.params.all_requirement = &packed_params.all_requirement;
		packed_params.params.setting = &packed_params.setting;

		//给出
		return packed_params;
	}
	//获得参数副本: 计算布局
	inline auto get_packed_params(const calculate_layout__params & params)
		-> calculate_layout__packed_params
	{
		//建立空变量
		calculate_layout__packed_params packed_params;

		//复制数据
		packed_params.requirement = *params.requirement;
		packed_params.all_requirement = *params.all_requirement;
		packed_params.setting = *params.setting;
		packed_params.params.window_size = params.window_size;

		//建立指针
		packed_params.params.requirement = &packed_params.requirement;
		packed_params.params.all_requirement = &packed_params.all_requirement;
		packed_params.params.setting = &packed_params.setting;

		//给出
		return packed_params;
	}
#pragma endregion
}