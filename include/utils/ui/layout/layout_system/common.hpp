#pragma once

#include <optional>
#include <algorithm>
#include <concepts>
#include <memory>
#include <utils/Range2D.hpp>
#include <utils/integer_type.hpp>
#include <utils/iterator.hpp>
#include <utils/value/property/Property.hpp>
#include <utils/value/property/ReadOnlyProperty.hpp>
#include <utils/value/dirty_value/DirtyValue.hpp>
#include <utils/value/dirty_value/DirtyFlag.hpp>
#include <utils/event/Event.hpp>
#include <utils/pointer/not_null.hpp>
#include <utils/pointer/nullable.hpp>
#include <utils/pointer/not_null_for_user.hpp>
#include <utils/debug/DebugNote.hpp>
#include <utils/vector.hpp>
#include <utils/ui/window/visit_manager.hpp>
#include <utils/ui/layout/OutsidePadding.hpp>
#include <utils/ui/layout/Align.hpp>

namespace utils::ui::layout::layout_system
{
	//布局需求中，表示 长/宽 不限
	inline const s8 LayoutUnlimitedValue = 100000;	//取个很大的，一般用不完的值，来模拟此窗口可以无限扩展

	//单方向需求
	struct SingleOrientationRequirement
	{
		s8 min_size {};
		s8 hint_size{};
		s8 max_size {};
	};

	//一个窗口的整体需求
	struct Requirement
	{
		utils::Index2D min_size;
		utils::Index2D hint_size;
		utils::Index2D max_size;

	public:
		//获取水平方向需求
		SingleOrientationRequirement get_horizontal() const
		{
			return { min_size.i, hint_size.i, max_size.i };
		}
		//获取垂直方向需求
		SingleOrientationRequirement get_vertical() const
		{
			return { min_size.j, hint_size.j, max_size.j };
		}
		//设置水平方向需求
		void set_horizontal(const SingleOrientationRequirement & requirement)
		{
			min_size .i = requirement.min_size ;
			hint_size.i = requirement.hint_size;
			max_size .i = requirement.max_size ;
		}
		//设置垂直方向需求
		void set_vertical(const SingleOrientationRequirement & requirement)
		{
			min_size .j = requirement.min_size ;
			hint_size.j = requirement.hint_size;
			max_size .j = requirement.max_size ;
		}
	};
	//一个窗口的所有的子窗口需求
	using AllRequirement = std::vector<Requirement>;

	//note: 把需要在 布局方法 中，访问的数据和函数，单独放入一个类，来拆解 布局管理器 和 布局方法 之间
	//      的相互调用
	class LayoutManager_1
		: public utils::ui::window::visit_manager::modifier::Visitable
	{
	protected:
		using VisitManager = utils::ui::window::visit_manager::modifier::VisitManager;
	public:
		utils::value::property::Property<utils::Index2D> m_window_position;	//窗口位置
		utils::value::property::Property<utils::Index2D> m_window_size;		//窗口大小
	protected:
		utils::value::dirty_value::DirtyValue<Requirement> m_requirement;	//窗口需求
		utils::value::dirty_value::DirtyValue<std::vector<Requirement>> m_all_requirement;	//子窗口需求
		utils::event::Event<void()> m_requirement_changed_event;	//事件: 需求已改变
		utils::value::dirty_value::DirtyFlag m_is_layout_dirty{true};	//脏标记: 布局已脏（需调用layout_ui函数恢复）
	public:
		//获得窗口遍历器
		//note: 给 布局方法 使用
		virtual VisitManager & _get_visit_manager() = 0;
		//获得需求改变事件
		//note: 给 布局方法 使用
		auto _get_requirement_changed_event() -> utils::event::Event<void()> &
		{
			return m_requirement_changed_event;
		}
	};

	//向前声明
	class LayoutManager;
}
namespace utils::ui::layout::layout_system::layout_method
{
	//布局方法
	class LayoutMethod
	{
	protected:
		friend class LayoutManager;

		utils::pointer::not_null_for_user<LayoutManager_1> m_layout_manager;
	private:
		friend class LayoutManager;

		//计算需求
		virtual Requirement calculate_requirement(const AllRequirement & all_requirement) const = 0;
		//计算布局
		virtual auto calculate_layout(utils::Index2D window_size, const Requirement & requirement,
			const AllRequirement & all_requirement)
			-> std::vector<utils::Range2D> = 0;
	};
}
//默认值
namespace utils::ui::layout::layout_system::default_value
{
	//note: 此值不是0大小，以方便用户在没有正确设置时，窗口还能显示出来，方便排错
	inline const Requirement & get_requirement()
	{
		static const Requirement requirement = {
			.min_size = {200,50},
			.hint_size = {200,50},
			.max_size = {200,50},
		};

		//给出
		return requirement;
	}
}