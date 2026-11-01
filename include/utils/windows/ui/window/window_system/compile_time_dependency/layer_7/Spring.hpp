#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	//弹簧
	//m_is_vertical: 是否为垂直弹簧
	template<bool m_is_vertical>
	class Spring final : public layer_6::Window
	{
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
			}

			//转发
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_CHILD;
		}
	private:
		//布局方式
		utils::pointer::not_null_for_user<layer_4::layout::method::leaf_layout::LeafLayout> m_layout_method;
	private:
		//处理 WM_CREATE 消息
		void on_message_create()
		{
			//设置布局方式
			{
				//准备布局方式
				auto layout_method = std::make_unique<layer_4::layout::method::leaf_layout::LeafLayout>();

				//在此类中，保存一份布局方式的指针，方便对其进行设置
				m_layout_method = layout_method.get();

				//设置布局方式
				m_layout_manager.set_layout_method(std::move(layout_method));
			}

			//设置布局需求
			{
				//准备数据: 布局需求(单方向)
				layer_4::SingleOrientationRequirement single_orientation_requirement;

				//最小大小为0
				single_orientation_requirement.min_size = 0;

				//最优大小为无限
				//note: 方便它能比正常控件，抢到更多的空间
				single_orientation_requirement.hint_size = layer_4::LayoutUnlimitedValue;

				//最大大小为无限
				single_orientation_requirement.max_size = layer_4::LayoutUnlimitedValue;

				//准备数据: 布局需求
				layer_4::Requirement requirement;

				//设置单方向需求
				{
					//垂直弹簧
					if constexpr (m_is_vertical)
					{
						//设置垂直方向的需求
						requirement.set_vertical(single_orientation_requirement);
					}
					//水平弹簧
					else
					{
						//设置水平方向的需求
						requirement.set_horizontal(single_orientation_requirement);
					}
				}

				//设置布局需求
				m_layout_method->set_requirement(requirement);
			}
		}
	};

	//垂直弹簧
	using VSpring = Spring<true>;
	//水平弹簧
	using HSpring = Spring<false>;
}