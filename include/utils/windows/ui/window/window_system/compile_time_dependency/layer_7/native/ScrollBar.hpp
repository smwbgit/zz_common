#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_7/native/WindowsStandardControl.hpp>
#include <utils/windows/ui/window/windows_standard_control/scroll_bar.hpp>
#include <utils/basic/store_this_integer.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7::native
{
	//滚动条
	//note: 因为往 create_window 函数里加参数比较难，所以这一版，先把 "是否为垂直滚动条" 这个参数，
	//		当做模版参数
	//m_is_vertical: 是否为垂直滚动条
	template<bool m_is_vertical>
	class ScrollBar final : public WindowsStandardControl
	{
	public:
		//是否已经和某个滚动视图绑定
		bool _m_is_bind{};

		//这四个值的说明，参见 utils::ui::layout::layout_system::layout_method::scroll_layout::single_orientation::Limited 里的相应变量
	public:
		utils::value::property::Property<s8> m_scroll_position{ 0 };
		utils::value::property::ReadOnlyProperty<utils::Range1D> m_scroll_range{ utils::Range1D{0,1} };
		utils::value::property::ReadOnlyProperty<s8> m_page_step{ 1 };
		utils::value::property::Property<std::optional<s8>> m_single_step;
	private:
		//native滚动条 的 滚动范围 发生改变
		utils::event::Event<void()> m_native_scroll_range_changed;
	public:
		ScrollBar()
		{
			m_scroll_position.add_checker([this](s8 & new_value)
			{
				//把滚动位置，限制在滚动范围中

				//获取滚动范围
				const auto scroll_range = m_scroll_range.get_value();

				//进行限制
				scroll_range.clamp_inplace(new_value);
			});
			m_scroll_range.add_assertor([this](const utils::Range1D & new_value)
			{
				//简单检查
				utils_assert(new_value.size >= 1);
			});
			m_page_step.add_assertor([this](const s8 & new_value)
			{
				//简单检查
				utils_assert(new_value >= 1);
			});
			m_single_step.add_assertor([this](const std::optional<s8> & new_value)
			{
				if(new_value.has_value())
				{
					//简单检查
					utils_assert(new_value.value() >= 1);
				}
			});

			//note: 因为不做修正时，滚动条和滚动视图，正好是方向相反的。
			//		这个版本，把滚动范围和滚动位置，在native里，直接给成他们的负数值，来模拟一个
			//		相反的滚动条。

			m_scroll_position += [this]()
			{
				//获取滚动位置
				const auto scroll_position = m_scroll_position.get_value();

				//转为windows表示
				const auto nPos = scroll_position * (-1);

				//绑定 native滚动条
				using ScrollBar = utils::windows::ui::window::windows_standard_control::scroll_bar::ScrollBar;
				ScrollBar scroll_bar;
				scroll_bar.m_id.set_by_standalone(get_native_hwnd());

				//更新 native滚动条 的 滚动位置
				scroll_bar.set_position(nPos);
			};
			m_scroll_range += [this]()
			{
				//native滚动条 的 滚动范围 发生改变
				m_native_scroll_range_changed.notify();
			};
			m_page_step += [this]()
			{
				//native滚动条 的 滚动范围 发生改变
				m_native_scroll_range_changed.notify();
			};

			m_native_scroll_range_changed += [this]()
			{
				//获取滚动范围
				const auto scroll_range = m_scroll_range.get_value();

				//获取页步长
				const auto page_step = m_page_step.get_value();

				//转为windows表示
#if 0
				const auto nMin = scroll_range.position;
				const auto nMax = scroll_range.get_last_value() + page_step - 1;
#else
				const auto nMin = scroll_range.get_last_value() * (-1);
				const auto nMax = scroll_range.position * (-1) + page_step - 1;
#endif
				const auto nPage = page_step;

				//绑定 native滚动条
				using ScrollBar = utils::windows::ui::window::windows_standard_control::scroll_bar::ScrollBar;
				ScrollBar scroll_bar;
				scroll_bar.m_id.set_by_standalone(get_native_hwnd());

				//设置滚动范围
				scroll_bar.set_range(nMin, nMax, nPage);
			};

		}
	private:
		DWORD get_native_style() const override final
		{
			if(m_is_vertical)
				return SBS_VERT;
			else
				return SBS_HORZ;
		}
		std::wstring get_native_class_name() const override final
		{
			return L"SCROLLBAR";
		}
	private:
		layer_4::Requirement get_layout_requirement() override final
		{
			//对于垂直滚动条
			//    宽度
			//        最小，最大，最优，都为 系统推荐值
			//    高度
			//        最小，最优，为推荐值的两倍
			//        最大，为无限
			//对于水平滚动条，同理

			//先按垂直滚动条算，如果是水平滚动条，则变换一下结果

			//准备数据
			layer_4::Requirement requirement;

			//系统推荐值
			const auto system_hint_size =
				utils::windows::ui::window::windows_standard_control::scroll_bar::
				get_system_hint_size();

			//宽度
			requirement.min_size.i = system_hint_size;
			requirement.hint_size.i = system_hint_size;
			requirement.max_size.i = system_hint_size;

			//高度
			requirement.min_size.j = system_hint_size * 2;
			requirement.hint_size.j = system_hint_size * 2;
			requirement.max_size.j = layer_4::LayoutUnlimitedValue;

			//水平滚动条
			if(!m_is_vertical)
			{
				//变换方向
				layer_4::layout::change_orientation::change_inplace(requirement);
			}

			//给出
			return requirement;
		}
	private:
		void on_message_create()
		{
			//手动激活下这个事件，来让滚动条，变成无滑块的滚动条
			m_native_scroll_range_changed.notify();
		}
		LRESULT wnd_proc_4(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
				on_message_create();
				break;
			case WM_VSCROLL:
			case WM_HSCROLL:
			{
				//简单检查
				utils_assert(m_is_vertical ? uMsg == WM_VSCROLL : uMsg == WM_HSCROLL);

				//获取单步进
				const auto single_step = m_single_step.get_value();

				//单步进
				const auto _single_step = single_step.value_or(1);	//当未被设置时，默认为1

				//准备函数
				auto check_scroll_position = [this](int & new_value)
				{
					//保存新值
					m_scroll_position.set_value(new_value * (-1));

					//然后再获取一下，以便于，如果被修正，可以获取修正后的值
					const auto _new_value = m_scroll_position.get_value() * (-1);

					//给出
					utils::basic::store_in(new_value, _new_value);
				};

				//转发
				return utils::windows::ui::window::windows_standard_control::scroll_bar::
					meet_window_message(hwnd, uMsg, wParam, lParam, _single_step,
						check_scroll_position);
			}
			}

			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	};
}