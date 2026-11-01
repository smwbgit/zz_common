#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/WindowsStandardControl.hpp>
#include <utils/windows/ui/window/windows_standard_control/static_text.hpp>
#include <utils/windows/ui/gdi/text/rect.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7::native
{
	//静态文本
	class StaticText final : public WindowsStandardControl
	{
	public:
		using TextAlignValue = utils::windows::ui::window::windows_standard_control::static_text::TextAlignValue;
	public:
		utils::value::property::Property<std::u8string> m_text;
		utils::value::property::Property<utils::color::Color> m_text_color{utils::color::White};
		utils::value::property::Property<utils::color::Color> m_text_background_color{utils::color::Black};
		utils::value::property::Property<TextAlignValue> m_text_align{TextAlignValue::Small};
	public:
		StaticText()
		{
			m_text += [this]()
			{
				//获取新值
				const auto text = m_text.get_value();

				//更改native控件
				s_hwnd(get_native_hwnd()).set_text(text);

				//更新布局需求
				update_layout_requirement();
			};
			m_text_color += [this]()
			{
				//让native控件重新绘制
				s_hwnd(get_native_hwnd()).invalidate_window();
			};
			m_text_background_color += [this]()
			{
				s_hwnd(get_native_hwnd()).invalidate_window();
			};
			m_text_align += [this]()
			{
				//获取新值
				const auto align = m_text_align.get_value();

				//更改native控件
				utils::windows::ui::window::windows_standard_control::static_text::
					set_text_align(get_native_hwnd(), align);
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

			//最优大小 设为 文本大小
			requirement.hint_size = text_size;

			//最优大小 补上 边框
			requirement.hint_size.i += text_size.j * 0.1;	//todo 看看这里怎么写

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
			return SS_LEFT;
		}
		std::wstring get_native_class_name() const override final
		{
			return L"STATIC";
		}
	private:
		LRESULT wnd_proc_4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CTLCOLORSTATIC:
			{
				HDC hdc = (HDC)wParam;

				//获取文字颜色
				const auto text_color = m_text_color.get_value();
				const auto _text_color = utils::windows::ui::rgb::from_color(text_color);

				//设置文字颜色
				utils_assert(CLR_INVALID != SetTextColor(hdc, _text_color));
        
				//设置背景颜色
#if 0
				if(m_is_text_transparent)
				{
					//透明
					utils_assert(SetBkMode(hdc, TRANSPARENT));
					const auto brush =
						utils::windows::ui::gdi::object::get_stock_object::brush(NULL_BRUSH);
					return (LRESULT)brush;
				}
				else
				{
#endif

					//因为文本的背景会被背景画刷填充，此处直接透明即可
					using s_hdc = utils::windows::ui::gdi::dc::s_hdc;
					s_hdc(hdc).set_background_transparent(true);

					//获取背景颜色
					const auto text_background_color = m_text_background_color.get_value();

					//生成背景画刷
					auto brush = utils::windows::ui::gdi::object::not_need_release::
						get_brush(text_background_color);

					//返回背景画刷
					return (LRESULT)brush;
#if 0
				}
#endif
				break;
			}
			}

			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	};
}