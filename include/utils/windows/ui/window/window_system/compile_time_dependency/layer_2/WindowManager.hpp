#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_1/Window.hpp>
#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_2/Thread.hpp>
#include <utils/windows/ui/window.hpp>
#include <utils/vector.hpp>
#include <utils/NextId.hpp>
#include <utils/pointer/nullable.hpp>
#include <utils/pointer/not_null.hpp>
#include <utils/event/Event.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_2
{
	//窗口管理器
	//note: 负责窗口的 创建/保存/销毁
	//note: 每个窗口线程上，都有一个窗口管理器，管理该线程上所有的主窗口
	//note: 每个主窗口上，也都有一个窗口管理器，管理该窗口上所有的子窗口
	//note: 每个子窗口上，也都有一个窗口管理器，管理该窗口上所有的子窗口
	//note: 管理主窗口的窗口管理器，没有父窗口，其值为0
	class WindowManager final
		: public utils::ui::window::visit_manager::modifier::VisitManager	//可以用它来遍历窗口
	{
	private:
		utils::pointer::nullable<layer_1::Window> m_window;
		utils::pointer::nullable<WindowManager> m_parent;
		std::vector<std::unique_ptr<layer_1::Window>> m_all_window;
	public:
		WindowManager()
		{
		}
		WindowManager(layer_1::Window & window)
			: m_window{ &window }
		{
		}
		~WindowManager()
		{
			//销毁所有窗口
			destroy_all();
		}
	public:
		WindowManager(const WindowManager &) = delete;
		WindowManager(WindowManager &&) = delete;
		WindowManager & operator =(const WindowManager &) = delete;
		WindowManager & operator =(WindowManager &&) = delete;
#pragma region 实现 访问管理器
	private:
		using Visitable = utils::ui::window::visit_manager::modifier::Visitable;
	private:
		//事件: 有窗口已被创建
		mutable utils::event::Event<void()> m_window_created_event;
	public:
		//获取事件: 有窗口已被创建
		utils::event::Event<void()> & v_get_window_created_event() const override final
		{
			return m_window_created_event;
		}
		bool v_is_root_window() const override final
		{
			//简单检查
			utils_assert(m_parent);

			return m_parent->m_parent == nullptr;
		}
		utils::pointer::nullable<Visitable> v_get_parent_window() const override final
		{
			//有父窗口
			if(m_parent && m_parent->m_window)
			{
				//给出
				return m_parent->m_window;
			}

			//简单检查: 此函数不是从 根管理器 发起调用的
			utils_assert(m_parent);

			//没有父窗口
			return {};
		}
		//获得根窗口
		//note: 如果当前窗口，是根窗口，则返回当前窗口
		//note: 如果从 根管理器 调用此函数，则报错。因为此位置，没有平级窗口
		Visitable & v_get_root_window() const
		{
			//简单检查
			utils_assert(m_parent);

			//向上查找，直到到达 根窗口 的 窗口管理器
			const WindowManager* current = this;
			while(1)
			{
				if(current->is_root_window())
					break;

				current = current->m_parent;
			}

			//给出
			return *current->m_window;
		}
		//获得所有直接子窗口
		//note: 不包括 没有融合进此窗口系统的 子窗口。比如，如果一个窗口自己用CreateWindow创建
		//      了子窗口A，那么A并不会在这个函数中被返回
		auto v_get_all_window() const
			-> std::vector<utils::pointer::not_null<Visitable>>
		{
			std::vector<utils::pointer::not_null<Visitable>> all_window;
			for(const auto & one : m_all_window)
			{
				all_window.push_back(one.get());
			}
			return all_window;
		}
#pragma endregion
#pragma region 创建窗口
	private:
		struct create_window__params_inner
		{
			WindowManager* _this{};	//因为要在静态函数中使用，所以加上this指针
			std::function<std::unique_ptr<layer_1::Window>(HWND hwnd)> create_window_object;
		};
	private:
		utils::NextId<uint32_t> m_window_next_id;
	public:
		template<typename window_T, typename... Args>
		auto create_window(Args&&... args) -> window_T &
		{
			//创建窗口
			const WNDPROC wnd_proc = WindowManager::wnd_proc;
			const std::wstring class_name = L"utils::windows::ui::window::window_system::compile_time_dependency::layer_2::WindowManager::Window";
			const DWORD window_style = window_T::get_default_style();
			const bool is_child_window = (m_window != nullptr);
			const uint32_t window_id = is_child_window ? m_window_next_id.get() : 0;
			const HWND parent_hwnd = is_child_window ? m_window->get_hwnd() : 0;

			//检查窗口样式
			check_window_style(window_style);

			//准备数据
			auto args_2 = std::tuple<Args...>(std::forward<Args>(args)...);

			create_window__params_inner params;
			params._this = this;
			params.create_window_object = [this, args_3 = std::move(args_2)](HWND hwnd)	//为了避免把wnd_proc变为模版函数，增加复杂性，所以，把类对象的创建，单独变成一个函数
			{
				//创建窗口对象
				auto window = std::apply([](auto && ... args) {
					return std::make_unique<window_T>(std::forward<decltype(args)>(args)...);
				}, std::move(args_3));

				//紧跟着赋值一些关键数据
				window->set_hwnd(hwnd);
				window->get_window_manager().m_parent = this;

				//添加调试信息
				window->_debug_note.add_not_existed(u8"c++ class", (const char8_t*)typeid(window_T).name());

				//给出
				return std::move(window);
			};
			const LPVOID lpParam = &params;

			utils::windows::ui::window::register_class__params register_params;
			register_params.is_allow_registered = true;
			register_params.lpfnWndProc = wnd_proc;
			register_params.cbWndExtra = sizeof(void*) * 1;
			register_params.class_name = class_name;

			utils::windows::ui::window::create_window__params create_params;
			create_params.dwStyle = window_style;
			create_params.class_name = class_name;
			create_params.parent_hwnd = parent_hwnd;
			create_params.menu = (HMENU)(uint64_t)window_id;
			create_params.lpParam = lpParam;
				
			utils::windows::ui::window::register_class(register_params);
			const auto hwnd = utils::windows::ui::window::create_window(create_params);

			//发出事件: 有窗口已被创建
			m_window_created_event.notify();

			//获得窗口对象
			const auto window = get_window_by_hwnd(hwnd);

			//转为真实类型
			auto & created_window = window->_down_cast<window_T>();

			//给出
			return created_window;
		}
	private:
		static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			//处理 创建/销毁 消息
			switch (uMsg)
			{
			case WM_NCCREATE:
			{
				//获取传入参数
				auto create_struct = (LPCREATESTRUCTW)lParam;
				auto params = (create_window__params_inner*)create_struct->lpCreateParams;

				//创建对象
				auto created_window = params->create_window_object(hwnd);

				//保存对象前，先获取一下它的指针
				const auto p_created_window = created_window.get();

				//保存对象到 管理器 中
				params->_this->m_all_window.push_back(std::move(created_window));

				//保存对象到 windows窗口 上
				s_hwnd(hwnd).set_window_extra_data(0, p_created_window);

				return TRUE;
			}
			case WM_DESTROY:
			{
				return 0;
			}
			case WM_NCDESTROY:
			{
				//获取窗口对象
				const auto window = get_window_by_hwnd(hwnd);

				//获取管理器
				auto & manager = window->get_window_manager();

				//简单检查：此窗口被销毁，是通过 管理器 触发的，而不是比如，系统通过WM_CLOSE自动触发的
				utils_assert(manager.m_is_destorying);

				//简单检查
				utils_assert(manager.m_parent);

				//窗口已在windows系统上销毁，只剩 管理器 中保存的窗口对象，删除它
				utils::vector(manager.m_parent->m_all_window).remove_unique_if(
					[window](const auto & one)  {
						return one.get() == window;  });
				return 0;
			}
			case WM_GETMINMAXINFO:
			{
				//获取窗口对象
				const auto window = get_window_by_hwnd(hwnd, true);

				//跳过 WM_NCCREATE 前的一条 WM_GETMINMAXINFO
				if(!window)
					return DefWindowProcW(hwnd, uMsg, wParam, lParam);
			}
			}

			//转发消息给窗口本身

			//获取窗口对象
			const auto window = get_window_by_hwnd(hwnd);
			return window->wnd_proc_1(hwnd, uMsg, wParam, lParam);
		}
		static layer_1::Window* get_window_by_hwnd(HWND hwnd, bool is_allow_nullptr = false)
		{
			const auto window = (layer_1::Window*)s_hwnd(hwnd).get_window_extra_data(0);

			if( !is_allow_nullptr )
			{
				//简单检查
				utils_assert(window);
			}

			return window;
		}
		//检查窗口样式是否正确
		void check_window_style(DWORD window_style)
		{
			//简单检查

			if(m_window)
			{
				//子窗口
				utils_assert((window_style & WS_CHILD) != 0);
			}
			else
			{
				//主窗口
				utils_assert((window_style & WS_CHILD) == 0);
			}
		}
#pragma endregion
#pragma region 销毁窗口
	private:
		bool m_is_destorying{};	//为了检测因误操作而导致的窗口意外被销毁的情况，而加的标志
	public:
		//销毁所有窗口，不包括和 管理器 关联的那个窗口
		void destroy_all()
		{
			//遍历所有窗口
			while(1)
			{
				if(m_all_window.empty())
					break;

				//最后一个窗口 (从后往前销毁)
				auto & window = *utils::vector(m_all_window).last_one().get();

				//销毁窗口
				destroy_window(window);
			}
		}
		//销毁指定窗口
		void destroy_window(layer_1::Window & window)
		{
			//获得 管理器
			auto & window_manager = window.get_window_manager();

			//把当前窗口以及子窗口以及子子窗口，都标记为 正在销毁
			window_manager.set_destorying_flag();

			//通知windows系统，销毁窗口
			utils_assert(DestroyWindow(window.get_hwnd()));
		}
		//销毁此窗口
		void destroy_window()
		{
			//简单检查: 还有上层的窗口管理器
			utils_assert(m_parent);

			//简单检查
			utils_assert(m_window);

			//让上层的窗口管理器，销毁此窗口
			m_parent->destroy_window(*m_window);
		}
	private:
		//添加标记: 正在销毁
		void set_destorying_flag()
		{
			//准备函数
			auto set_flag = [](bool & is_destorying)
			{
				//简单检查
				utils_assert(!is_destorying);

				//标记为 正在销毁
				is_destorying = true;
			};

			//标记当前窗口
			set_flag(m_is_destorying);

			//标记子窗口/子子窗口
			for(const auto & one : m_all_window)
			{
				one->get_window_manager().set_destorying_flag();
			}
		}
#pragma endregion
	private:
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
	};
}