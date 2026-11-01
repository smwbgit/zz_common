#pragma once

#include <utils/ui/layout/layout_system/common.hpp>

namespace utils::ui::layout::layout_system
{
	class LayoutManager : public LayoutManager_1
	{
	private:
		using LayoutMethod = layout_method::LayoutMethod;
	public:
		//窗口遍历器
		utils::pointer::not_null_for_user<VisitManager> m_visit_manager;
	private:
		//布局方法
		std::optional<std::unique_ptr<LayoutMethod>> m_layout_method;
	private:
		VisitManager & _get_visit_manager() override final
		{
			return *m_visit_manager;
		}
	private:
		//只有根窗口，才会使用此组值
		struct s_only_root
		{
			//所有，因需求改变，而需要重新布局，但还未被处理的窗口
			std::vector<utils::pointer::not_null<LayoutManager>> all_source_window;
			//重入标志：正在进行布局
			bool is_layouting{};
		};
		s_only_root m_only_root;
	public:
		LayoutManager()
		{
		}
		//初始化
		void init()
		{
			//窗口大小 改变后，需要重新布局
			m_window_size += [this]()
			{
				//根窗口
				if(m_visit_manager->is_root_window())
				{
					//用户修改根窗口大小，而引起的窗口大小改变

					//重新布局
					layout_root_ui();
				}
				//非根窗口
				else
				{
					//本布局系统在布局时，引起的窗口大小改变

					//把本窗口的布局，标记为已脏
					dirty_layout();

					//重新布局：不用再管父窗口
					//note: 这一版本先这样
					//todo: 看看是否需要这一行。毕竟 _layout_ui 函数里，会再次调用一次所有
					//		子窗口的 _layout_ui 函数
					_layout_ui();
				}
			};

			//布局参数 改变后，需要重新布局
			m_requirement_changed_event += [this]()
			{
				//重新布局
				layout_root_ui();
			};

			//获取事件: 有窗口已被创建
			auto & window_created_event = m_visit_manager->get_window_created_event();

			//有窗口已被创建后，需要重新布局
			window_created_event += [this]()
			{
				//重新布局
				layout_root_ui();
			};
		}
		//设置布局方式
		void set_layout_method(std::unique_ptr<LayoutMethod> layout_method)
		{
			//简单检查
			utils_assert(layout_method);

			//添加信息
			layout_method->m_layout_manager = this;

			//保存
			m_layout_method = std::move(layout_method);

			//发出通知（因为布局方法发生变化后，窗口的布局需求也会发生变化，除非前后的布局方法和所有参数都一样）
			m_requirement_changed_event.notify();
		}
		//获取布局方式
		//return: 当还未设置布局方式时,返回空值
		utils::pointer::nullable<LayoutMethod> get_layout_method()
		{
			//还未设置布局方式
			if (!m_layout_method)
				return {};

			//简单检查
			utils_assert(m_layout_method.value());

			//给出
			return {&*m_layout_method.value()};
		}
		//获取最优大小
		const utils::Index2D & get_hint_size()
		{
			//获取需求
			const auto & requirement = get_requirement();

			//最优大小
			const auto & hint_size = requirement.hint_size;

			//给出
			return hint_size;
		}
	private:
		//重新布局
		//note: 此函数，是所有由外部改变（比如，布局参数，根窗口的大小被用户调整，有新窗口创建）而
		//		引起的重新布局的总入口。
		//note: 此函数，由各层的窗口，在所在层，发起调用。内部只是简单的转发给根窗口处理。
		void layout_root_ui()
		{
			//根窗口
			auto & root_window = m_visit_manager->get_root_window<LayoutManager>();

			//让根窗口重新布局
			root_window._layout_root_ui(*this);
		}
		//在根窗口上，重新布局
		//note: 不要直接调用此函数，而是调用 layout_root_ui 函数
		//source_window: 是哪个窗口的布局需求发生了变化，而引起的此函数被调用
		void _layout_root_ui(LayoutManager & source_window)
		{
			/*  为了支持，布局可重入（也就是，在layout_root_ui函数中又调用了它），设计了这样一套流程，
				大致是：

				首先，在本窗口布局系统中，只有两种情况，会引起窗口重新布局：
					一个是某个窗口的布局需求发生了变化
					另一个，是根窗口被用户拖拽，根窗口的大小发生了变化
				其次，窗口的每次重新布局，（不包括标脏动作），都是从根窗口开始，逐级向下的。
				所以，让根窗口的窗口大小发生改变事件里，不去调用layout_ui，而是调用layout_root_ui，
					来达到，所有的布局动作，都有一个统一的入口函数。

				layout_root_ui函数，只能在根窗口上调用。也就是，调用它的唯一方式，是:
					根窗口->layout_root_ui()

				在需求变化事件里（根窗口 和 非根窗口）：
					调用 根窗口->layout_root_ui(当前窗口)

				同样，在根窗口的窗口大小发生改变事件里：
					调用 根窗口->layout_root_ui(当前窗口)
				这种方式，会进行，重新计算，根窗口的需求，（仅根窗口这一层），这样的多余计算。
				但为了能简化代码，这一版，先这样。

				在layout_root_ui函数里：
					把参数（发生了需求改变的那个窗口）放入一个数组中。
					如果当前在重入（通过检查一个bool变量），则直接返回。
					否则：
						设置重入标记
						A:  从这个数组中，弹出一个待处理的子窗口
							给这个子窗口往上的窗口们，加脏标志
							然后发起一次根窗口布局(通过调用layout_ui函数)
						重复A动作，直到这个数组中，没有待处理的子窗口为止。
						取消重入标记
			*/

			//简单检查: 当前窗口，为根窗口
			utils_assert(m_visit_manager->is_root_window());

			//定义一个名字更短的名称，不然后边的名字太长啦
			auto & r = m_only_root;

			//把发起窗口，放入待处理窗口数组里。如果已经存在，则不再重复添加，因为最后的结果是一样的
			//    （同样的需求，不需要布局两遍）
			utils::vector(r.all_source_window).push_back_if_not_exist(&source_window);

			//如果此次调用是重入
			if(r.is_layouting)
			{
				//直接返回，让第一次函数调用，来处理数组中，新加入的项
				return;
			}

			//添加标记：布局正在进行
			r.is_layouting = true;

			//进行重复动作
			while(1)
			{
				//简单检查
				utils_assert(!utils::vector(r.all_source_window).empty());

				//拿出一个待处理的窗口。从前面还是从后面拿，无所谓，因为效果是一样的。
				//这里从后面拿，因为vector比较容易从后边拿。
				const auto source_window = utils::vector(r.all_source_window).pop_back();

				//给这个窗口，添加一直到根窗口的脏标记
				source_window->dirty_requirement();

				//发起一次，从根窗口的布局
				_layout_ui();

				//都已处理完
				if(utils::vector(r.all_source_window).empty())
					break;
			}

			//取消 正在处理 标记
			r.is_layouting = false;
		}
		//在当前窗口上，重新布局
		//note: 不要直接调用此函数，此函数是为了配合 layout_root_ui 函数，完成整个布局的更新
		void _layout_ui()
		{
			//布局未脏
			if(!m_is_layout_dirty)
			{
				//无操作
				return;
			}

			//取消脏标志
			m_is_layout_dirty = false;

			//还没有指定布局方式
			if(!m_layout_method.has_value())
			{
				//不进行布局
				return;
			}

			//获取需求
			const auto & requirement = get_requirement();

			//获取子窗口需求
			const auto & all_requirement = get_all_requirement();
			
			//获取窗口大小
			const auto window_size = m_window_size.get_value();

			//获取布局方式
			auto & layout_method = *m_layout_method.value();

			//计算布局
			//note: 这里传入 当前窗口的布局需求，方便 布局方法中直接使用，而不用再计算一遍。
			const auto rects = layout_method.calculate_layout(window_size, requirement, all_requirement);

			//放置子窗口
			put_all_window(rects);

			//再手动触发一遍所有的子窗口的布局。以防止，“子窗口的布局需求改变了，但恰巧分配给它的窗口
			//    空间没变，而导致的布局调用链断掉”这类的情况

			//获取所有子窗口
			const auto all_window = m_visit_manager->get_all_window<LayoutManager>();

			//对于每个子窗口
			for(auto & window : all_window)
			{
				//重新布局
				window->_layout_ui();
			}
		}
		//获取布局需求
		const Requirement & get_requirement()
		{
			//还没有指定布局方式
			if(!m_layout_method.has_value())
			{
				//给出一个默认布局需求
				return default_value::get_requirement();
			}

			//数据已脏，重新生成
			if(m_requirement.is_dirty())
			{
				//获取子窗口需求
				const auto & all_requirement = get_all_requirement();

				//获取布局方式
				auto & layout_method = *m_layout_method.value();

				//计算总需求
				const auto requirement = layout_method.calculate_requirement(all_requirement);

				//简单检查
				utils_assert(is_valid_requirement(requirement));

				//保存
				m_requirement.set_value(std::move(requirement));
			}

			//给出
			return m_requirement.get_value();
		}
#pragma region 此类的辅助函数
	private:
		//是否为有效的布局需求
		static bool is_valid_requirement(const Requirement & requirement)
		{
			if (requirement.min_size.i <= requirement.hint_size.i &&
				requirement.max_size.i >= requirement.hint_size.i &&
				requirement.min_size.j <= requirement.hint_size.j &&
				requirement.max_size.j >= requirement.hint_size.j )
			{
				return true;
			}
			else
				return false;
		}
		//把 本窗口和父窗口和父父窗口 的 需求/子窗口需求/布局，这三个，标记为已脏
		//note: 之所以，标记本窗口的需求变脏时，要往上找，直到根窗口，是因为，本窗口需求变化，
		//      从这里往上的所有需求，都会发生变化
		//note: 之所以，标记本窗口的需求变脏时，要标记 需求/子窗口需求/布局 这三个，是因为，
		//      需求一变，布局就需要重排，父窗口的子窗口需求也要更新一下。唯一的多余更新，是
		//      本窗口的子窗口需求，这一个。但为了代码简洁，就也把它顺带更新了。
		void dirty_requirement()
		{
			//从本窗口开始
			LayoutManager* current_window = this;

			while(1)
			{
				//添加脏标记: 需求
				current_window->m_requirement.set_dirty();
				//添加脏标记: 子窗口需求
				current_window->m_all_requirement.set_dirty();
				//添加脏标记: 布局
				current_window->m_is_layout_dirty = true;

				//获取遍历管理器
				auto & window_visit_manager = *current_window->m_visit_manager;

				//变为父窗口
				current_window = window_visit_manager.get_parent_window<LayoutManager>();

				//根窗口也添加完毕
				if(!current_window)
					break;
			}
		}
		//获取子窗口需求
		const AllRequirement & get_all_requirement()
		{
			//数据已脏，重新生成
			if(m_all_requirement.is_dirty())
			{
				//获取所有的子窗口
				const auto all_window = m_visit_manager->get_all_window<LayoutManager>();

				//准备数据
				AllRequirement all_requirement;

				//查询每个子窗口
				for(const auto & window : all_window)
				{
					//获取需求
					const auto & requirement = window->get_requirement();

					//保存需求
					all_requirement.push_back(requirement);
				}

				//保存
				m_all_requirement.set_value(std::move(all_requirement));
			}

			//给出
			return m_all_requirement.get_value();
		}
		//把本窗口的布局，标记为已脏
		void dirty_layout()
		{
			m_is_layout_dirty = true;
		}
		//放置子窗口
		void put_all_window(const std::vector<utils::Range2D> & rects)
		{
			//获取所有子窗口
			const auto all_window = m_visit_manager->get_all_window<LayoutManager>();

			//简单检查
			utils_assert(rects.size() == all_window.size());

			//设置每个子窗口的矩形
			for(const auto i : utils::iterator_index(rects))
			{
				//子窗口
				auto & window = *all_window[i];

				//设置位置
				window.m_window_position.set_value(rects[i].position);

				//设置大小
				window.m_window_size.set_value(rects[i].size);
			}
		}
#pragma endregion

	public:
		utils::debug::DebugNote _debug_note;
	};
}