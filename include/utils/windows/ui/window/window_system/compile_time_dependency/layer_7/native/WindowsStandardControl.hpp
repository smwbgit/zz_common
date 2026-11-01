#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>

//native: 把windows标准控件，融入此窗口系统中
namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7::native
{
	//windows标准控件
	//note: 创建一个空的窗口，把windows的标准控件创建在它的里面。
	//      这样就可以处理那些windows标准控件报告给父窗口的消息
	class WindowsStandardControl : public layer_6::Window
	{
	private:
		std::optional<HWND> m_native_hwnd;
		utils::pointer::not_null_for_user<layer_4::layout::method::leaf_layout::LeafLayout> m_layout_method;

#pragma region 此类的核心函数
	private:
		//处理 WM_CREATE 消息
		void on_message_create()
		{
			//创建标准控件
			create_native_window();

			//设置布局，要在设置字体之前。因为不然的话，设置字体后，连带的更新布局需求，
			//    会被设置到旧的布局方式上

			//设置布局方式
			set_layout_method();

			//设置字体相关
			set_for_font();
		}
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
			{
				on_message_create();
				break;
			}
			case WM_SIZE:
			{
				//获取新的窗口大小
				const auto new_size = s_hwnd(hwnd).get_client_size();
            
				//简单检查
				utils_assert(m_native_hwnd);

				//设置子窗口大小
				s_hwnd(*m_native_hwnd).set_size(new_size);

				//todo 看看到底需不需要再调用 DefWindowProc
				return 0;
			}
			}

			//转发
			return wnd_proc_4(hwnd, uMsg, wParam, lParam);
		}
		virtual LRESULT wnd_proc_4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
#pragma endregion

	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_CHILD | WS_VISIBLE;
		}
	protected:
		HWND get_native_hwnd() const
		{
			//简单检查
			utils_assert(m_native_hwnd);

			return *m_native_hwnd;
		}
		auto get_layout_method() const
			-> layer_4::layout::method::leaf_layout::LeafLayout &
		{
			//简单检查
			utils_assert(m_layout_method);

			return *m_layout_method;
		}
	private:
		virtual DWORD get_native_style() const = 0;
		virtual std::wstring get_native_class_name() const = 0;
	private:
		//获取布局需求
		virtual layer_4::Requirement get_layout_requirement() = 0;
	protected:
		//更新布局需求
		void update_layout_requirement()
		{
			//获取需求
			const auto requirement = get_layout_requirement();

			//设置需求
			get_layout_method().set_requirement(requirement);
		}
	protected:
		//标准控件的控件id
		const u4 m_native_control_id = 1;

#pragma region 此类的辅助函数
	private:
		//创建标准控件
		void create_native_window()
		{
			//准备参数
			utils::windows::ui::window::create_window__params create_params;
			create_params.dwStyle = WS_CHILD | WS_VISIBLE | get_native_style();
			create_params.class_name = get_native_class_name();
			create_params.parent_hwnd = get_hwnd();
			create_params.menu = (HMENU)(uint64_t)m_native_control_id;

			//创建子窗口
			m_native_hwnd = utils::windows::ui::window::create_window(create_params);
		}
		//设置布局方式
		void set_layout_method()
		{
			//准备布局方式
			auto layout_method = std::make_unique<layer_4::layout::method::leaf_layout::LeafLayout>();

			//在此类中，保存一份布局方式的指针，方便对其进行设置
			m_layout_method = layout_method.get();

			//设置布局方式
			m_layout_manager.set_layout_method(std::move(layout_method));
		}
		//设置字体相关
		void set_for_font()
		{
			//字体改变的事件处理
			auto meet_font_changed = [this]()
			{
				//获取字体
				const auto font = m_font_manager.m_font.get_value();

				//设置字体
				s_hwnd(get_native_hwnd()).set_font(font);

				//更新布局需求
				update_layout_requirement();
			};

			//添加事件处理
			m_font_manager.m_font.add_listener(meet_font_changed);

			//手动触发一次事件处理，以便于，设置native控件的字体，以及由此而来的，对布局需求的更新。
			//    之所以需要手动触发，是因为，字体管理器属于被动监听，不能用set_font在此窗口上设置
			//    字体，不然会被认为在此级窗口上，有用户手工指定的字体设置。
			//这个版本先手动调用 事件处理，而不是交给字体管理器的某个函数来统一处理这种情况
			meet_font_changed();
		}
#pragma endregion
	};
}