#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_2/WindowManager.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_3/Window.hpp>
#include <utils/ui/layout/layout_system/LayoutManager.hpp>
#include <utils/ui/layout/layout_system/layout_method/leaf_layout/LeafLayout.hpp>
#include <utils/ui/layout/layout_system/layout_method/box_layout/BoxLayout.hpp>
#include <utils/ui/layout/layout_system/layout_method/scroll_layout/ScrollLayout.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4::layout
{
	//布局方法
	namespace method
	{
		//叶子布局
		namespace leaf_layout
		{
			using namespace utils::ui::layout::layout_system::layout_method::leaf_layout;
		}
		//线性布局
		namespace box_layout
		{
			using namespace utils::ui::layout::layout_system::layout_method::box_layout;
		}
		//滚动布局
		namespace scroll_layout
		{
			using namespace utils::ui::layout::layout_system::layout_method::scroll_layout;
		}
	}
	//横竖变换
	namespace change_orientation
	{
		using namespace utils::ui::layout::layout_system::compute::change_orientation;
	}
}
namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4
{
	//向前声明
	class Window;

	//布局需求中，表示 长/宽 不限
	inline const auto LayoutUnlimitedValue = utils::ui::layout::layout_system::LayoutUnlimitedValue;

	//布局需求
	using Requirement = utils::ui::layout::layout_system::Requirement;

	//布局需求: 单方向
	using SingleOrientationRequirement = utils::ui::layout::layout_system::SingleOrientationRequirement;

	//桥梁类
	//note: 不负责布局的任何任务，只负责，把 utils::ui::layout::layout_manager::LayoutManager
	//      和当前的窗口系统联结在一起
	class LayoutManager
	{
	private:
		using VisitManager = utils::ui::window::visit_manager::modifier::VisitManager;
		using LayoutMethod = utils::ui::layout::layout_system::layout_method::LayoutMethod;
		using LayoutManager_Real = utils::ui::layout::layout_system::LayoutManager;
	private:
		layer_3::Window & m_window;
		LayoutManager_Real m_layout_manager;	//真正负责布局的
		std::unique_ptr<VisitManager> m_visit_manager;	//帮助 真正布局器 在窗口树中遍历
	public:
		//设置布局方式
		void set_layout_method(std::unique_ptr<LayoutMethod> layout_method)
		{
			//转发
			return m_layout_manager.set_layout_method(std::move(layout_method));
		}
		//特化
		template<typename LayoutMethod_T>
		LayoutMethod_T & set_layout_method()
		{
			//准备数据
			auto layout = std::make_unique<LayoutMethod_T>();
			auto & layout_2 = *layout;

			//转发
			set_layout_method(std::move(layout));

			//给出
			return layout_2;
		}
		//获取布局方式
		//return: 当还未设置布局方式时,返回空值
		utils::pointer::nullable<LayoutMethod> get_layout_method()
		{
			//转发
			return m_layout_manager.get_layout_method();
		}
		//特化
		template<typename LayoutMethod_T>
		LayoutMethod_T & get_layout_method()
		{
			//获取
			auto layout = get_layout_method();

			//简单检查
			utils_assert(layout);

			//简单转换
			auto layout_2 = dynamic_cast<LayoutMethod_T*>(&*layout);
			
			//简单检查
			utils_assert(layout_2);

			//给出
			return *layout_2;
		}
		//获取最优大小
		const utils::Index2D & get_hint_size()
		{
			//转发
			return m_layout_manager.get_hint_size();
		}
	private:
		friend class layer_4::Window;

		LayoutManager(layer_3::Window & window)
			: m_window{ window }
		{
			//生成一个窗口遍历器
			m_visit_manager = make_visit_manager();

			//给真正布局器，设置 窗口遍历器
			m_layout_manager.m_visit_manager = m_visit_manager.get();

			//初始化真正的布局管理器
			m_layout_manager.init();

			//窗口位置
			m_layout_manager.m_window_position += [this]()
			{
				//根窗口
				if(m_window.m_window_manager.is_root_window())
				{
					//简单检查
					utils_throw();
				}
				//不是根窗口
				else
				{
					//获取新值
					const auto position = m_layout_manager.m_window_position.get_value();

					//设为新值
					s_hwnd(m_window.get_hwnd()).set_position(position);
				}
			};
			
			//窗口大小
			m_layout_manager.m_window_size += [this]()
			{
				//根窗口
				if(m_window.m_window_manager.is_root_window())
				{
					//忽略
				}
				//不是根窗口
				else
				{
					//获取新值
					const auto size = m_layout_manager.m_window_size.get_value();

					//设为新值
					s_hwnd(m_window.get_hwnd()).set_size(size);
				}
			};
		}
		void meet_window_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			//根窗口
			if(m_window.m_window_manager.is_root_window())
			{
				//获取新值
				const auto client_size = s_hwnd(m_window.get_hwnd()).get_client_size();

				//设置新值
				m_layout_manager.m_window_size.set_value(client_size);
			}
			//不是根窗口
			else
			{
				//忽略
			}
		}
		//使用现有窗口遍历器，生成一个新的窗口遍历器。以真正的布局管理器为目标。
		auto make_visit_manager() -> std::unique_ptr<VisitManager>
		{
			//当前目标
			using _Window = layer_3::Window;

			//新目标
			using _Member = LayoutManager_Real;

			//准备函数: 从 当前目标 获取到 新目标
			auto get_member = [](_Window & window) -> _Member &
			{
				//获取窗口的布局管理器
				auto & layout_manager = window.get_layout_manager();

				//获取里面真正的布局管理器
				auto & _layout_manager = layout_manager.m_layout_manager;

				//给出
				return _layout_manager;
			};

			//当前的访问管理器
			auto & visit_manager = m_window.m_window_manager;

			//生成新的访问管理器
			auto _visit_manager = visit_manager.make_visit_manager<_Window, _Member>(get_member);

			//给出
			return std::move(_visit_manager);
		}
	public:
		void _add_debug_note(const std::u8string & key, const std::u8string & value)
		{
			m_layout_manager._debug_note.add_not_existed(key, value);
		}
	private:
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
	};
}