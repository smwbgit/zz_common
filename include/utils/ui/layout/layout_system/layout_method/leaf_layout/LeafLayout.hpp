#pragma once

#include <utils/ui/layout/layout_system/common.hpp>

namespace utils::ui::layout::layout_system::layout_method::leaf_layout
{
	//叶布局
	//note: 没有子窗口，直接指定总需求
	class LeafLayout : public LayoutMethod
	{
	private:
		Requirement m_requirement = default_value::get_requirement();

	public:
		//设置需求
		void set_requirement(const Requirement & requirement)
		{
			//保存
			m_requirement = requirement;

			//获取 需求改变事件
			auto & requirement_changed_event = m_layout_manager->_get_requirement_changed_event();

			//发出通知
			requirement_changed_event.notify();
		}
		//特化: 根据内容大小,算出需求
		void set_requirement(utils::Index2D content_size)
		{
			//获取需求
			const auto requirement = [&]()
				-> Requirement
			{
				//准备数据
				Requirement requirement;

				//计算大小
				requirement.hint_size = content_size;
				requirement.min_size = content_size;
				requirement.max_size = content_size;

				//给出
				return requirement;
			}();

			//转发
			return set_requirement(requirement);
		}

	private:
		Requirement calculate_requirement(const AllRequirement & all_requirement) const override final
		{
			//简单检查
			utils_assert(all_requirement.empty());

			//给出
			return m_requirement;
		}
		auto calculate_layout(utils::Index2D window_size, const Requirement & /* requirement */,
			const AllRequirement & all_requirement)
			-> std::vector<utils::Range2D> override final
		{
			//简单检查
			utils_assert(all_requirement.empty());

			//给出
			return {};
		}
	};
}