#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>
#include <utils/windows/ui/gdi/text/rect.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>
#include <utils/windows/ui/gdi/begin_paint.hpp>
#include <utils/ui/layout/OutsidePadding.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	//静态文本
	class StaticText final : public layer_6::Window
	{
	public:
		//简便写法
		using AlignValue = utils::ui::layout::AlignValue;
	public:
		//文本内容
		utils::value::property::Property<std::u8string> m_text;
		//文本颜色
		utils::value::property::Property<utils::color::Color> m_text_color{utils::color::Black};
		//背景颜色
		utils::value::property::Property<utils::color::Color> m_background_color{utils::color::White};
		//外边距
		utils::value::property::Property<utils::ui::layout::OutsidePadding> m_outside_padding;
		//对齐方式
		utils::value::property::Property<utils::ui::layout::Align> m_align;
	private:
		//布局方法
		utils::pointer::not_null_for_user<layer_4::layout::method::leaf_layout::LeafLayout> m_layout_method;
	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_CHILD | WS_VISIBLE;
		}
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
				on_message_create();
				break;
			case WM_SIZE:
				on_message_size();
				break;
			case WM_PAINT:
				on_message_paint();
				break;
			case WM_ERASEBKGND:
				return 1;	//已擦除
			}
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		//响应 WM_CREATE 消息
		void on_message_create()
		{
			//当文本改变时
			m_text += [this]()
			{
				//更新布局需求
				update_layout_requirement();

				//重新绘制
				get_s_hwnd().invalidate_window();
			};
			//当文本颜色改变时
			m_text_color += [this]()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			};
			//当背景颜色改变时
			m_background_color += [this]()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			};
			//当外边距改变时
			m_outside_padding += [this]()
			{
				//更新布局需求
				update_layout_requirement();

				//重新绘制
				get_s_hwnd().invalidate_window();
			};
			//当对齐方式改变时
			m_align += [this]()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			};

			//设置布局方式
			{
				//准备布局方式
				auto layout_method = std::make_unique<layer_4::layout::method::leaf_layout::LeafLayout>();

				//在此类中，保存一份布局方式的指针，方便对其进行设置
				m_layout_method = layout_method.get();

				//设置布局方式
				m_layout_manager.set_layout_method(std::move(layout_method));
			}
			
			//设置字体相关
			{
				//字体改变的事件处理
				auto meet_font_changed = [this]()
				{
					//更新布局需求
					update_layout_requirement();

					//重新绘制
					get_s_hwnd().invalidate_window();
				};

				//添加事件处理
				m_font_manager.m_font.add_listener(meet_font_changed);

				//手动触发一次事件处理，理由参见“layer_5::native::WindowsStandardControl”的
				//    “set_for_font”函数
				meet_font_changed();
			}
		}
		//响应 WM_SIZE 消息
		void on_message_size()
		{
			//重新绘制
			get_s_hwnd().invalidate_window();
		}
		//响应 WM_PAINT 消息
		void on_message_paint()
		{
			//窗口句柄
			const auto hwnd = get_hwnd();

			//绘制
			utils::windows::ui::gdi::begin_paint::with_memdc(hwnd, [&](HDC hdc)
			{
				//s_hdc
				utils::windows::ui::gdi::dc::s_hdc s_hdc{ hdc };

				//获取背景色
				const auto background_color = m_background_color.get_value();

				//填充背景
				s_hdc.draw_rectangle_solid(background_color);

				//获取字体
				const auto font = m_font_manager.m_font.get_value();

				//获取文本
				const auto text = m_text.get_value();

				//获取文本颜色
				const auto text_color = m_text_color.get_value();

				//绘制位置
				const auto text_position = [&]()
					-> utils::Index2D
				{
					//获取总大小
					const auto size = get_s_hwnd().get_client_size();

					//获取文本大小
					const auto text_size = get_text_layout_size();

					//获取外边框
					const auto outside_padding = m_outside_padding.get_value();

					//获取对齐方式
					const auto align = m_align.get_value();

					//多余大小
					const auto rest_size = size - text_size - outside_padding.get_size();

					//简单检查
					utils_assert(rest_size.i >= 0);
					utils_assert(rest_size.j == 0);

					//简便声明
					using AlignValue = utils::ui::layout::AlignValue;

					//文本的x坐标
					const auto x = [&]()
						-> s8
					{
						//为左对齐
						if(align.horizontal == AlignValue::Small)
						{
							//给出
							return outside_padding.left;
						}
						//为右对齐
						if(align.horizontal == AlignValue::Large)
						{
							//给出
							return outside_padding.left + rest_size.i;
						}
						//为居中对齐
						if(align.horizontal == AlignValue::Middle)
						{
							//给出
							return outside_padding.left + rest_size.i/2;
						}
						//抛出异常
						utils_throw();
					}();

					//文本的y坐标
					const auto y = outside_padding.top;

					//给出
					return {x, y};
				}();

				//绘制文本
				s_hdc.draw_text_transparent(text, text_position, font, text_color);
			});
		}
	private:
		//获取文本的布局大小
		utils::Index2D get_text_layout_size()
		{
			//算法：主要是调用  utils::windows::ui::gdi::text::rect::get 函数。
			//		在文本为空时，换成一个单字符，而不是空字符串，来计算。

			//获取文本
			const auto text = [&]()
				-> std::u8string
			{
				//获取文本
				const auto text = m_text.get_value();

				//文本为空
				if(text.empty())
				{
					//给出一个字符，以便于，能保持，“静态文本”的高度。
					return u8"1";
				}

				//给出
				return text;
			}();

			//获取字体
			const auto font = m_font_manager.m_font.get_value();

			//获取文本大小
			const auto text_size = utils::windows::ui::gdi::text::rect::
				get(text, font, get_hwnd());

			//给出
			return text_size;
		}
		//更新布局需求
		void update_layout_requirement()
		{
			//获取需求
			const auto requirement = [this]()
				-> layer_4::Requirement
			{
				//准备数据
				layer_4::Requirement requirement;

				//先算出 最优大小

				//获取文本大小
				const auto text_size = get_text_layout_size();

				//获取外边框
				const auto outside_padding = m_outside_padding.get_value();

				//最优大小 设为 文本大小
				requirement.hint_size = text_size;

				//最优大小 补上 外边框
				requirement.hint_size += outside_padding.get_size();

				//最小大小 = 最优大小
				requirement.min_size = requirement.hint_size;

				//最大大小 = 最优大小，且长度不限
				requirement.max_size = requirement.hint_size;
				requirement.max_size.i = layer_4::LayoutUnlimitedValue;

				//给出
				return requirement;
			}();

			//获取布局方法
			auto & layout_method = [this]()
				-> layer_4::layout::method::leaf_layout::LeafLayout &
			{
				//简单检查
				utils_assert(m_layout_method);

				return *m_layout_method;
			}();

			//设置需求
			layout_method.set_requirement(requirement);
		}
	};
}