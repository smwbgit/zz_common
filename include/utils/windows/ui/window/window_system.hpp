#pragma once

#include <windows.h>
#include <functional>
#include <memory>
#include <thread>
#include <utils/vector.hpp>
#include <utils/queue.hpp>
#include <utils/map.hpp>
#include <utils/NextId.hpp>
#include <utils/thread/AtomicObject.hpp>
#include <utils/thread/Flag.hpp>
#include <utils/windows/ui/window/message_loop.hpp>
#include <utils/windows/ui/window/layout_manager.hpp>
#include <utils/windows/ui/window.hpp>
#include <utils/value/EventedValue.hpp>
#include <utils/event/chain_callback/ChainCallbacks.hpp>
#include <utils/windows/ui/gdi/object/get_stock_object.hpp>
#include <utils/windows/ui/gdi/object/not_need_release.hpp>
#include <utils/windows/ui/gdi/ManagedHandle.hpp>
#include <utils/windows/ui/gdi/object/select.hpp>

namespace utils::windows::ui::window::window_system
{
	class BasicWindow
	{
	protected:
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
	public:
		const HWND m_hwnd{};
	public:
		BasicWindow(HWND hwnd)
			: m_hwnd{ hwnd }
		{
		}
		virtual ~BasicWindow()
		{
		}
	};

	//提供消息循环线程
	class Thread
	{
	private:
		utils::thread::AtomicObject<std::queue<std::function<void()>>> m_tasks;
		std::unique_ptr<std::thread> m_thread;
		utils::thread::Flag m_thread_exit_flag;
		std::vector<std::unique_ptr<BasicWindow>> m_main_windows;
	public:
		Thread()
		{
			//启动线程
			m_thread = std::make_unique<std::thread>([this](){
				thread_proc(); });
		}
		~Thread()
		{
			//销毁所有窗口
			call_in_thread([this]()
			{
				for(const auto & one : m_main_windows)
				{
					utils_assert(DestroyWindow(one->m_hwnd));
				}
			});

			//结束循环
			m_thread_exit_flag.set_true();

			//等待结束
			m_thread->join();

			//简单检查: 没有要创建的窗口
			utils_assert_abort(m_tasks.lock()->empty());
			
			//简单检查: 没有窗口存在
			utils_assert_abort(m_main_windows.empty());
		}
		//在此线程中 创建窗口/销毁窗口
		void call_in_thread(std::function<void()> func)
		{
			utils::thread::Flag flag;
			m_tasks.lock()->push([func, &flag](){
				func();
				flag.set_true(); });
			flag.wait();
		}
		void add_main_window(std::unique_ptr<BasicWindow> window)
		{
			//简单检查
			utils_assert(window);

			//保存窗口
			m_main_windows.push_back(std::move(window));
		}
		void remove_main_window(BasicWindow* window)
		{
			//简单检查
			utils_assert(window);

			//删除窗口
			utils::vector(m_main_windows).remove_unique_if(
				[window](const auto & one)  {
					return one.get() == window;  });
		}
	private:
		//线程函数
		void thread_proc()
		{
			utils::windows::ui::window::message_loop::MessageLoop message_loop;
			//在空闲时创建窗口
			message_loop.m_meet_idle = [this]() {
				while(!m_tasks.lock()->empty())
				{
					auto task = utils::queue(*m_tasks.lock()).pop();
					task();
				}  };
			message_loop.m_thread_exit_flag = &m_thread_exit_flag;
			message_loop.start();
		}
	};

	class MainWindow : public BasicWindow
	{
	private:
		struct s_temp_create_params
		{
			Thread* thread{};
			bool is_overlapped_window{};
			utils::thread::Flag finish{};
			MainWindow* created_window{};
		};
	public:
		//is_overlapped_window:  创建WS_OVERLAPPED窗口，还是WS_POPUP窗口
		static MainWindow & create(Thread & thread, bool is_overlapped_window = true)
		{
			//准备数据
			s_temp_create_params temp_create_params{};
			temp_create_params.thread = &thread;
			temp_create_params.is_overlapped_window = is_overlapped_window;

			//创建窗口
			thread.call_in_thread([&temp_create_params]() {

				WNDPROC _wnd_proc = wnd_proc;
				std::wstring class_name = L"utils::windows::ui::window::manage_window_by_object::MainWindow";
				DWORD window_style = temp_create_params.is_overlapped_window ? WS_OVERLAPPED : WS_POPUP;
				LPVOID lpParam = &temp_create_params;

				utils::windows::ui::window::register_class__params register_params;
				register_params.is_allow_registered = true;
				register_params.lpfnWndProc = _wnd_proc;
				register_params.cbWndExtra = sizeof(void*);
				register_params.class_name = class_name;

				utils::windows::ui::window::create_window__params create_params;
				create_params.dwStyle = window_style;
				create_params.class_name = class_name;
				create_params.lpParam = lpParam;
				
				utils::windows::ui::window::register_class(register_params);
				utils::windows::ui::window::create_window(create_params);
			});

			//等待窗口创建完成
			temp_create_params.finish.wait();

			//给出
			return *temp_create_params.created_window;
		}
		static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_NCCREATE:
			{
				//获取传入的自定义参数
				auto create_struct = (LPCREATESTRUCTW)lParam;
				auto temp_create_params = (s_temp_create_params*)create_struct->lpCreateParams;

				//创建对象
				auto created_window = std::make_unique<MainWindow>(
					hwnd, temp_create_params->thread);

				//保存对象
				auto p_created_window = created_window.get();
				temp_create_params->thread->add_main_window(std::move(created_window));

				//把对象指针保存在窗口上
				SetWindowLongPtrW(hwnd, 0, (LONG_PTR)p_created_window);

				//返回结果
				temp_create_params->created_window = p_created_window;
				temp_create_params->finish.set_true();
				return TRUE;
			}
			case WM_DESTROY:
			{
				return 0;
			}
			case WM_NCDESTROY:
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd);

				//删除对象
				_this->m_thread->remove_main_window(_this);
				return 0;
			}
			case WM_SIZE:
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd);

				//进行布局
				for(auto & layout : _this->m_layouts)
					layout->position_controls();
				return 0;
			}
			case WM_COMMAND:
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd);

				uint32_t control_id = LOWORD(wParam);
				if(utils::map(_this->m_command_handlers).is_exist(control_id))
				{
					return _this->m_command_handlers[control_id](wParam, lParam);
				}
				break;
			}
			case WM_GETMINMAXINFO:
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd, true);

				//跳过 WM_NCCREATE 前的一条 WM_GETMINMAXINFO
				if(!_this)
					return DefWindowProcW(hwnd, uMsg, wParam, lParam);
			}
			case WM_TIMER:
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd);

				//准备数据
				const auto timer_id = (uint32_t)wParam;

				//跳过id为0的。id为0时的消息，似乎是给系统的某部分使用的
				if (0 == timer_id)
					return DefWindowProcW(hwnd, uMsg, wParam, lParam);

				//准备数据
				const auto meet_expired = 
					utils::map(_this->m_timer_handlers).get_existed_value(timer_id);

				//转发
				meet_expired();
				return 0;
			}
			}

			//转发消息给子窗口
			{
				//获取对象指针
				auto _this = get_this_pointer(hwnd);

				if(utils::map(_this->m_message_handlers).is_exist(uMsg))
				{
					const auto & chain_callbacks = _this->m_message_handlers[uMsg];

					const auto result = chain_callbacks.call(wParam, lParam);
					if(result.has_value())
						return result.value();
				}
			}

			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		static MainWindow* get_this_pointer(HWND hwnd, bool is_allow_nullptr = false)
		{
			auto _this = (MainWindow*)GetWindowLongPtrW(hwnd, 0);

			//简单检查
			if(!is_allow_nullptr)
				utils_assert(_this);

			return _this;
		}
	public:
		Thread* const m_thread{};
		utils::NextId<uint32_t> m_control_next_id{};
	private:
		std::vector<std::unique_ptr<BasicWindow>> m_controls;
		std::map<uint32_t, std::function<LRESULT(WPARAM wParam, LPARAM lParam)>> m_command_handlers;
		std::map<UINT, utils::event::chain_callback::ChainCallbacks<LRESULT, WPARAM, LPARAM>> m_message_handlers;
		std::vector<std::unique_ptr<utils::windows::ui::window::layout_manager::BasicLayout>> m_layouts;
		utils::NextId<uint32_t> m_timer_next_id{};
		std::map<uint32_t, std::function<void()>> m_timer_handlers;
	public:
		MainWindow(HWND hwnd, Thread* thread)
			: BasicWindow{ hwnd }
			, m_thread{ thread }
		{
		}
		~MainWindow()
		{
		}
		void add_timer(std::chrono::milliseconds time_length, std::function<void()> meet_expired)
		{
			//简单检查
			utils_assert(time_length.count() > 0);

			//准备数据
			const auto timer_id = m_timer_next_id.get();
			
			//添加定时器
			utils_assert(SetTimer(m_hwnd, timer_id, time_length.count(), NULL));

			//保存回调函数
			m_timer_handlers[timer_id] = meet_expired;
		}
		void add_control(std::unique_ptr<BasicWindow> control)
		{
			//简单检查
			utils_assert(control);

			m_controls.push_back(std::move(control));
		}
		void add_command_handler(uint32_t control_id, std::function<LRESULT(WPARAM wParam, LPARAM lParam)> meet_command)
		{
			utils::map(m_command_handlers).insert_not_exist(control_id, meet_command);
		}
		void add_message_handler(UINT msg_id, std::function<std::optional<LRESULT>(WPARAM wParam, LPARAM lParam)> meet_message)
		{
			m_message_handlers[msg_id].add(meet_message);
		}
		void add_layout(std::unique_ptr<utils::windows::ui::window::layout_manager::BasicLayout> layout)
		{
			//简单检查
			utils_assert(layout);

			m_layouts.push_back(std::move(layout));
		}
	};

	class BasicControlWindow : public BasicWindow
	{
	protected:
		template<typename control_T>
		struct create_control__params
		{
			MainWindow* window{};
			DWORD dwStyle{};
			std::wstring class_name;
			std::function<LRESULT(control_T* window, uint32_t command_id)> command_handler;
			std::map<UINT, std::function<std::optional<LRESULT>(control_T* window, WPARAM, LPARAM)>> message_handlers;
		};
		template<typename control_T>
		static auto create_control(const create_control__params<control_T> & params) -> control_T &
		{
			//准备数据
			struct s_temp_create_params
			{
				const create_control__params<control_T>* p{};
				utils::thread::Flag finished{};
				control_T* created_window{};
			};
			s_temp_create_params temp_create_params{};
			temp_create_params.p = &params;

			//创建窗口
			params.window->m_thread->call_in_thread([&temp_create_params]() {

				//创建窗口

				DWORD dwStyle = WS_VISIBLE | WS_CHILD | temp_create_params.p->dwStyle;
				std::wstring class_name = temp_create_params.p->class_name;
				HWND parent_hwnd = temp_create_params.p->window->m_hwnd;
				uint32_t control_id = temp_create_params.p->window->m_control_next_id.get();
				auto command_handler = temp_create_params.p->command_handler;
				const auto & message_handlers = temp_create_params.p->message_handlers;
				auto parent_window = temp_create_params.p->window;

				utils::windows::ui::window::create_window__params create_params;
				create_params.dwStyle = dwStyle;
				create_params.class_name = class_name;
				create_params.parent_hwnd = parent_hwnd;
				create_params.menu = (HMENU)(uint64_t)control_id;

				HWND hwnd = utils::windows::ui::window::create_window(create_params);

				//保存窗口

				auto created_window = std::make_unique<control_T>(hwnd);

				auto p_created_window = created_window.get();
				parent_window->add_control(std::move(created_window));

				//处理自己的消息

				if(command_handler)
				{
					parent_window->add_command_handler(control_id, [control_id, p_created_window, command_handler](WPARAM wParam, LPARAM lParam)->LRESULT
					{
						//简单检查
						utils_assert(LOWORD(wParam) == control_id);

						return command_handler(p_created_window, HIWORD(wParam));
					});
				}
				for(const auto & message_handler : message_handlers)
				{
					const auto msg_id = message_handler.first;
					const auto callback = message_handler.second;
					parent_window->add_message_handler(msg_id, [callback, p_created_window](WPARAM wParam, LPARAM lParam) -> std::optional<LRESULT>
					{
						return callback(p_created_window, wParam, lParam);
					});
				}

				//给出结果

				temp_create_params.created_window = p_created_window;
				temp_create_params.finished.set_true();
			});

			//等待窗口创建完成
			temp_create_params.finished.wait();

			//给出
			return *temp_create_params.created_window;
		}
	public:
		BasicControlWindow(HWND hwnd)
			: BasicWindow{ hwnd }
		{
		}
	};

	class CheckBox : public BasicControlWindow
	{
	private:
		struct s_temp_create_params
		{
			MainWindow* window{};
			utils::thread::Flag finished{};
			CheckBox* created_window{};
		};
	public:
		utils::value::EventedValue<bool> m_is_checked;
	public:
		static CheckBox & create(MainWindow & window)
		{
			create_control__params<CheckBox> params{};
			params.window = &window;
			params.dwStyle = BS_AUTOCHECKBOX;
			params.class_name = L"BUTTON";
			params.command_handler = [](CheckBox* window, uint32_t command_id) -> LRESULT
			{
				switch(command_id)
				{
				case BN_CLICKED:
				{
					bool is_checked = SendMessageW(window->m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
					window->m_is_checked.set(is_checked);
					return 0;
				}
				default:
					utils_throw();
				}
			};

			return create_control<CheckBox>(params);
		}
	public:
		void add_listener_is_checked_changed(std::function<void(const bool & is_checked)> listener)
		{
			utils_assert(!listener);

			m_is_checked.add_changed_listener(listener);
		}
	public:
		CheckBox(HWND hwnd)
			: BasicControlWindow{ hwnd }
		{
		}
	};

	class StaticText : public BasicControlWindow
	{
	public:
		static StaticText & create(MainWindow & window)
		{
			auto message_handler_1 = [](StaticText* window, WPARAM wParam, LPARAM lParam)
				-> std::optional<LRESULT>
			{
				HDC hdc = (HDC)wParam;

				//设置文字颜色
				utils_assert(CLR_INVALID != SetTextColor(hdc, window->m_text_color));
        
				//设置背景颜色
				if(window->m_is_text_transparent)
				{
					//透明
					utils_assert(SetBkMode(hdc, TRANSPARENT));
					const auto brush =
						utils::windows::ui::gdi::object::get_stock_object::brush(NULL_BRUSH);
					return (LRESULT)brush;
				}
				else
				{
					utils_assert(SetBkMode(hdc, TRANSPARENT));
					auto brush = utils::windows::ui::gdi::object::not_need_release::get_brush(
						window->m_text_background_color);
					return (LRESULT)brush;
				}
			};

			create_control__params<StaticText> params{};
			params.window = &window;
			params.dwStyle = SS_LEFT;
			params.class_name = L"STATIC";
			params.command_handler = nullptr;
			params.message_handlers = {{WM_CTLCOLORSTATIC, message_handler_1}};

			return create_control<StaticText>(params);
		}
	private:
		std::atomic<COLORREF> m_text_color{RGB(0,0,0)};
		std::atomic<COLORREF> m_text_background_color{RGB(255,255,255)};
		std::atomic<bool> m_is_text_transparent{};
	public:
		StaticText(HWND hwnd)
			: BasicControlWindow{ hwnd }
		{
		}
		void set_text(const std::u8string & text)
		{
			s_hwnd(m_hwnd).set_text(text);
		}
		//获得能包裹住文本的最小大小
		utils::Index2D get_recommended_size()
		{
			//获得text
			const auto text = s_hwnd(m_hwnd).get_text();

			//获得hdc
			const auto hdc = GetDC(m_hwnd);
			utils_assert(hdc);
			utils::windows::ui::gdi::ManagedHandle<HDC> _hdc{hdc, m_hwnd};
			
			//获得font
			HFONT font = (HFONT)SendMessageW(m_hwnd, WM_GETFONT, 0, 0);
			utils_assert(font);
			
			//使用font
			auto auto_select_old = utils::windows::ui::gdi::object::select::font(hdc, font);

			//准备数据
			const auto text_u16 = utils::text::encoding::unicode::utf8_to_utf16(text);

			//查询
			SIZE text_size{};
			utils_assert(GetTextExtentPoint32W(
				hdc, (const wchar_t*)text_u16.c_str(), (int)text_u16.length(), &text_size));

			return {text_size.cx, text_size.cy};
		}
		void right_align()
		{
			s_hwnd(m_hwnd).modify_style(SS_RIGHT, SS_LEFT | SS_CENTER);
		}
		void set_text_color(COLORREF color)
		{
			m_text_color = color;
		}
		void set_text_transparent(bool is_transparent)
		{
			m_is_text_transparent = is_transparent;
		}
		void set_text_background_color(COLORREF color)
		{
			m_text_background_color = color;
		}
	};
}