#pragma once

#include <vector>
#include <utils/modifier/Modifier.hpp>
#include <utils/pointer/nullable.hpp>
#include <utils/pointer/not_null.hpp>
#include <utils/event/Event.hpp>

namespace utils::ui::window::visit_manager::modifier
{
	//一个可以被上下遍历的窗口
	class Visitable : public utils::modifier::Modifier
	{
	};

	//访问管理器
	//note: 抽象类
	class VisitManager : public utils::modifier::Modifier
	{
#pragma region 此类的核心虚函数
		//note: 因为相同的名称会在继承时被遮掩，所以特意改为和模版函数不同的名字
	public:
		//获取根窗口
		virtual Visitable & v_get_root_window() const = 0;
		//获取父窗口
		//return：如果没有父窗口，则返回空
		virtual utils::pointer::nullable<Visitable> v_get_parent_window() const = 0;
		//获取所有的子窗口
		virtual std::vector<utils::pointer::not_null<Visitable>> v_get_all_window() const = 0;
		//当前窗口是否为根窗口
		virtual bool v_is_root_window() const = 0;
		//获取事件: 有窗口已被创建
		virtual utils::event::Event<void()> & v_get_window_created_event() const = 0;
#pragma endregion

#pragma region 此类的辅助函数
	public:
		//特化
		utils::event::Event<void()> & get_window_created_event() const
		{
			//转发
			return v_get_window_created_event();
		}
		//特化
		bool is_root_window() const
		{
			//转发
			return v_is_root_window();
		}
		//特化
		template<typename T>
		T & get_root_window()
		{
			//转发
			auto & root_window = v_get_root_window();

			//向下转换
			auto & root_window_2 = dynamic_cast<T &>(root_window);

			//给出
			return root_window_2;
		}
		//特化
		template<typename T>
		utils::pointer::nullable<T> get_parent_window()
		{
			//转发
			const auto parent_window = v_get_parent_window();

			//向下转换
			auto parent_window_2 = dynamic_cast<T*>(parent_window.get());

			//给出
			return parent_window_2;
		}
		//特化
		template<typename T>
		std::vector<utils::pointer::not_null<T>> get_all_window()
		{
			//转发
			const auto all_window = v_get_all_window();

			std::vector<utils::pointer::not_null<T>> all_window_2;

			for(const auto & window : all_window)
			{
				//向下转换
				auto window_2 = dynamic_cast<T*>(window.get());

				all_window_2.push_back({window_2});
			}

			//给出
			return all_window_2;
		}
#pragma endregion
	public:
		//生成一个成员访问管理器
		//note: 依旧是一个访问管理器，依旧是原来的那颗窗口树。只是把目标，变为原来窗口的某个成员
		//note: “成员”是指，窗口的某个成员。由外部用户，使用get_member回调函数指定
		//note: 要求此成员，也继承了 Visitable 这个修饰词
		//note: 函数体后置
		template<typename Window, typename Member>
		auto make_visit_manager(std::function<Member &(Window & window)> get_member)
			-> std::unique_ptr<VisitManager>;
	};

	namespace inner::new_visit_manager
	{
		//成员访问管理器
		template<typename Window, typename Member>
		class MemberVisitManager final : public VisitManager
		{
			//简单检查
			static_assert(std::is_base_of_v<Visitable, Window>);
			static_assert(std::is_base_of_v<Visitable, Member>);

		private:
			VisitManager & m_visit_manager;
			std::function<Member &(Window & window)> m_get_member;
		public:
			MemberVisitManager(
				VisitManager & visit_manager,
				std::function<Member &(Window & window)> get_member)
				: m_visit_manager{ visit_manager }
				, m_get_member{ get_member }
			{
			}
		public:
			Visitable & v_get_root_window() const override final
			{
				//获取根窗口
				auto & root_window = m_visit_manager.get_root_window<Window>();

				//获取成员
				auto & member = m_get_member(root_window);

				//给出
				return member;
			}
			utils::pointer::nullable<Visitable> v_get_parent_window() const override final
			{
				//获取父窗口
				auto parent_window = m_visit_manager.get_parent_window<Window>();

				//不存在父窗口
				if(!parent_window)
				{
					//返回空值
					return {};
				}

				//获取成员
				auto & member = m_get_member(*parent_window);

				//给出
				return &member;
			}
			std::vector<utils::pointer::not_null<Visitable>> v_get_all_window() const override final
			{
				//获取子窗口
				auto all_window = m_visit_manager.get_all_window<Window>();

				//最终结果
				std::vector<utils::pointer::not_null<Visitable>> _all_window;

				//对于每个子窗口
				for(auto & window : all_window)
				{
					//获取成员
					auto & member = m_get_member(*window);

					//保存
					_all_window.push_back(&member);
				}

				//给出
				return _all_window;
			}
			bool v_is_root_window() const override final
			{
				//转发
				return m_visit_manager.is_root_window();
			}
			utils::event::Event<void()> & v_get_window_created_event() const override final
			{
				//转发
				return m_visit_manager.get_window_created_event();
			}
		};
	}

	//给出函数体
	template<typename Window, typename Member>
	auto VisitManager::make_visit_manager(std::function<Member &(Window & window)> get_member)
		-> std::unique_ptr<VisitManager>
	{
		//类型
		using MemberVisitManager = inner::new_visit_manager::MemberVisitManager<Window, Member>;

		//创建
		auto visit_manager = std::make_unique<MemberVisitManager>(*this, get_member);

		//给出
		return std::move(visit_manager);
	}
}