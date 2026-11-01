#pragma once

#include <utils/ui/layout/layout_system/layout_method/scroll_layout/common.hpp>
#include <utils/ui/layout/layout_system/layout_method/scroll_layout/SingleOrientation.hpp>

namespace utils::ui::layout::layout_system::layout_method::scroll_layout
{
	//滚动布局
	class ScrollLayout : public LayoutMethod
	{
	public:
		//水平方向
		single_orientation::Limited m_horizontal;
		//垂直方向
		single_orientation::Limited m_vertical;

	private:
		utils::event::Event<void()> m_scroll_position_changed;
	public:
		ScrollLayout()
		{
			m_horizontal.m_scroll_position += [this]()
			{
				m_scroll_position_changed.notify();
			};
			m_vertical.m_scroll_position += [this]()
			{
				m_scroll_position_changed.notify();
			};
			m_scroll_position_changed += [this]()
			{
				//如果滚动位置发生改变，直接平移窗口，而不触发重新布局，以便减少计算量

				//获取全部子窗口
				auto children = m_layout_manager->_get_visit_manager().get_all_window<LayoutManager_1>();

				//简单检查
				utils_assert(children.size() <= 1);

				//没有子窗口
				if(children.empty())
				{
					//无需操作
					return;
				}

				//子窗口
				auto & child = *children[0];

				//获取子窗口位置
				const auto x = m_horizontal.m_scroll_position.get_value();
				const auto y = m_vertical  .m_scroll_position.get_value();

				//子窗口位置
				const utils::Index2D position{ x,y };

				//设置位置 (通过布局管理器)
				child.m_window_position.set_value(position);
			};
		}

#pragma region 此类的核心函数
	private:
		//计算需求
		Requirement calculate_requirement(const AllRequirement & all_requirement) const override final
		{
			//没有子窗口
			if(all_requirement.empty())
			{
				//这里暂时不通知两个方向子量，等计算布局时，再通知

				//正常的界面，不会需要这个返回值。但这里返回一个大一点的区域，以便于撑起当前窗口，
				//    方便界面排版时的排错。
				return default_value::get_requirement();
			}

			//简单检查
			utils_assert(all_requirement.size() == 1);

			//子窗口需求
			const auto & child_requirement = all_requirement[0];

			//子窗口需求的方向分量
			const auto child_requirement_horizontal = child_requirement.get_horizontal();
			const auto child_requirement_vertical   = child_requirement.get_vertical();

			//交给各方向处理
			const auto requirement_horizontal =
				m_horizontal.calculate_requirement(child_requirement_horizontal);
			const auto requirement_vertical   =
				m_vertical  .calculate_requirement(child_requirement_vertical  );

			//拼出完整需求
			Requirement requirement;
			requirement.set_horizontal(requirement_horizontal);
			requirement.set_vertical  (requirement_vertical  );

			//给出
			return requirement;
		}
		//计算布局
		auto calculate_layout(utils::Index2D window_size, const Requirement & /* requirement */,
			const AllRequirement & all_requirement)
			-> std::vector<utils::Range2D> override final
		{
			//没有子窗口
			if(all_requirement.empty())
			{
				//通知两个方向子量，没有子窗口
				m_horizontal.now_no_child();
				m_vertical  .now_no_child();

				//不需要布局
				return {};
			}

			//简单检查
			utils_assert(all_requirement.size() == 1);

			//子窗口需求
			const auto & child_requirement = all_requirement[0];

			//子窗口需求的方向分量
			const auto child_requirement_horizontal = child_requirement.get_horizontal();
			const auto child_requirement_vertical   = child_requirement.get_vertical();

			//转发给 方向分量 来计算
			const auto rect_horizontal =
				m_horizontal.calculate_layout(window_size.i, child_requirement_horizontal);
			const auto rect_vertical   =
				m_vertical  .calculate_layout(window_size.j, child_requirement_vertical  );

			//拼出结果
			utils::Range2D rect;
			rect.position.i = rect_horizontal.position;
			rect.position.j = rect_vertical  .position;
			rect.size.i = rect_horizontal.size;
			rect.size.j = rect_vertical  .size;

			//给出
			return {rect};
		}
#pragma endregion
	};
}