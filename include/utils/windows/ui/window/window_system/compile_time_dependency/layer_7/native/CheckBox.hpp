#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/WindowsStandardControl.hpp>
#include <utils/windows/ui/window/windows_standard_control/check_box.hpp>
#include <utils/windows/ui/gdi/text/rect.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7::native
{
	//复选框
	class CheckBox final : public WindowsStandardControl
	{
	public:
		utils::value::property::Property<std::u8string> m_text;
		utils::value::property::Property<bool> m_is_checked;
	public:
		CheckBox()
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
			m_is_checked += [this]()
			{
				//获取状态
				const auto is_checked = m_is_checked.get_value();

				//改变ui
				namespace check_box = utils::windows::ui::window::windows_standard_control::check_box;
				check_box::set_checked( get_native_hwnd(), is_checked );
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

			//最优大小 补充上 方块大小
			requirement.hint_size.i += 19 + (u8)(text_size.j * 0.2);	//todo 看看这里怎么写,是否有标准写法

			//如果文本太小，放不下前面的方块，则扩大一下高度
			if(requirement.hint_size.j < 16)
				requirement.hint_size.j = 16;	//todo 看看这里有没有标准算法，来求出它的高度

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
			return BS_AUTOCHECKBOX;
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
				const uint32_t control_id = LOWORD(wParam);
				const uint32_t command_id = HIWORD(wParam);

				//简单检查
				utils_assert(control_id == m_native_control_id);

				switch(command_id)
				{
				case BN_CLICKED:
				{
					//查询状态
					const bool is_checked =
						utils::windows::ui::window::windows_standard_control::check_box::
						is_checked( get_native_hwnd() );

					//保存
					m_is_checked.set_value(is_checked);

					//不再转发
					return 0;
				}
				default:
					utils_throw();
				}

				//转发没处理的
				break;
			}
			}

			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	};
}