#pragma once

#include <utils/ui/layout/layout_system/layout_method/box_layout/common.hpp>
#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/Compute.hpp>

namespace utils::ui::layout::layout_system::layout_method::box_layout
{
	//线性布局
	class BoxLayout : public LayoutMethod
	{
	private:
		Setting m_setting;

	public:
		BoxLayout()
		{
			//设置默认值
			m_setting.is_vertical_layout = true;
			m_setting.align.horizontal = AlignValue::Small;
			m_setting.align.vertical = AlignValue::Small;
			m_setting.inside_padding.value = 20;
			m_setting.outside_padding.left = 20;
			m_setting.outside_padding.right = 20;
			m_setting.outside_padding.top = 20;
			m_setting.outside_padding.bottom = 20;
		}

#pragma region 给用户使用的函数
	public:
		//一次设置所有选项
		void set_all_setting(Setting setting)
		{
			//保存
			m_setting = setting;

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
		//设置布局方向
		void set_orientation(bool is_vertical_layout)
		{
			//保存
			m_setting.is_vertical_layout = is_vertical_layout;

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
		//设置外边距
		void set_outside_padding(OutsidePadding outside_padding)
		{
			//保存
			m_setting.outside_padding = outside_padding;

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
		//特化
		void set_outside_padding(s8 outside_padding)
		{
			//保存
			m_setting.outside_padding = {
				outside_padding,
				outside_padding,
				outside_padding,
				outside_padding};

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
		//设置内边距
		void set_inside_padding(InsidePadding inside_padding)
		{
			//保存
			m_setting.inside_padding = inside_padding;

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
		//设置对齐方向
		void set_align(Align align)
		{
			//保存
			m_setting.align = align;

			//发出通知
			m_layout_manager->_get_requirement_changed_event().notify();
		}
#pragma endregion

#pragma region 此类的核心函数
	private:
		//计算需求
		Requirement calculate_requirement(const AllRequirement & all_requirement) const override final
		{
			//没有子窗口
			if(all_requirement.empty())
			{
				//正常的界面，不会需要这个返回值。但这里返回一个大一点的区域，以便于撑起当前窗口，
				//    方便界面排版时的排错。
				return default_value::get_requirement();
			}

			//准备参数
			compute::Compute::calculate_requirement__params params;
			params.all_requirement = &all_requirement;
			params.setting = &m_setting;

			//计算需求
			const auto requirement = compute::Compute::calculate_requirement(params);

			//给出
			return requirement;
		}
		//计算布局
		auto calculate_layout(utils::Index2D window_size, const Requirement & requirement,
			const AllRequirement & all_requirement)
			-> std::vector<utils::Range2D> override final
		{
			//没有子窗口
			if(all_requirement.empty())
			{
				//不需要布局
				return {};
			}

			//准备参数
			compute::Compute::calculate_layout__params params;
			params.window_size = window_size;
			params.requirement = &requirement;
			params.all_requirement = &all_requirement;
			params.setting = &m_setting;

			//计算布局
			const auto rects = compute::Compute::calculate_layout(params);

			//给出
			return rects;
		}
#pragma endregion
	};
}