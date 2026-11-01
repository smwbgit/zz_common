#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/WindowsStandardControl.hpp>
#include <utils/windows/ui/gdi/text/rect.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7::native
{
	//按钮
	class Button final : public WindowsStandardControl
	{
	public:
		//按钮名称
		utils::value::property::Property<std::u8string> m_text;
		//事件: 按钮被点击
		utils::event::Event<void()> m_clicked_event;
	public:
		Button()
		{
			m_text += [this]()
			{
				//获取新值
				const auto text = m_text.get_value();

				//更改native控件
				s_hwnd(get_native_hwnd()).set_text(text);

				//更新布局要求
				update_layout_requirement();
			};
		}
	private:
		layer_4::Requirement get_layout_requirement() override final
		{
			//准备数据
			layer_4::Requirement requirement;

			//先算出 最优大小

			//获得文本
			const auto text = m_text.get_value();
			
			//获得字体
			const auto font = m_font_manager.m_font.get_value();

			//获得文本大小
			const auto text_size = utils::windows::ui::gdi::text::rect::get(text, font, get_native_hwnd());

			//最优大小 先设为 文本大小
			requirement.hint_size = text_size;

			//最优大小 补充上 文字旁边的边距
			requirement.hint_size.i += 4 + (u8)(text_size.j * 1.3);	//todo 看看这里怎么写,是否有标准写法
			requirement.hint_size.j += 4 + (u8)(text_size.j * 0.5);

			//到此 最优大小 已完成

			//最小大小 = 最优大小
			requirement.min_size = requirement.hint_size;

			//最大大小 = 最优大小，且长度不限
			requirement.max_size = requirement.hint_size;
			requirement.max_size.i = layer_4::LayoutUnlimitedValue;

			//给出
			return requirement;
		}
	private:
		DWORD get_native_style() const override final
		{
			return 0;
		}
		std::wstring get_native_class_name() const override final
		{
			return L"BUTTON";
		}
	private:
		LRESULT wnd_proc_4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_COMMAND:
			{
				//准备数据
				const u4 control_id = LOWORD(wParam);
				const u4 command_id = HIWORD(wParam);

				//简单检查
				utils_assert(control_id == m_native_control_id);

				switch(command_id)
				{
				case BN_CLICKED:
				{
					//触发事件
					m_clicked_event.notify();

					//不再转发
					return 0;
				}
				default:
					//简单检查
					utils_throw();
				}

				//简单检查
				utils_throw();
			}
			}

			//转发
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	};
}