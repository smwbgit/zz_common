#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/basic/bits.hpp>
#include <utils/windows/ui/window/s_hwnd.hpp>	//for debug

namespace utils::windows::ui::window::windows_standard_control::scroll_bar
{
	//滚动条标识
	//note: 能唯一标识一个滚动条，不论它是垂直还是水平，是内置还是独立
	class ScrollBarID
	{
	public:
		bool m_is_system{};
		HWND m_hwnd{};
		UINT m_flag{};
	public:
		//通过独立条的 hwnd ，来设置此结构
		void set_by_standalone(HWND hwnd)
		{
			m_is_system = false;
			m_hwnd = hwnd;
			m_flag = SB_CTL;
		}
		//通过 WM_VSCROLL/WM_HSCROLL 消息的参数，来设置此结构
		void set_by_window_message(HWND hwnd, UINT uMsg, LPARAM lParam)
		{
			//拆解参数
			const HWND control = (HWND)lParam;   // 0 表示内置条

			//独立条
			if(control)
			{
				m_is_system = false;
				m_hwnd = control;
				m_flag = SB_CTL;
			}
			//内置条
			else
			{
				m_is_system = true;
				m_hwnd = hwnd;
				m_flag = uMsg == WM_VSCROLL ? SB_VERT : SB_HORZ;
			}
		}
	};
	//代表一个滚动条
	class ScrollBar
	{
	public:
		ScrollBarID m_id;
		bool m_is_redraw = true;	//SetScrollInfo 是否重新绘制
		bool m_is_disable_no_scroll = true;	//当不需要滚动时，是否显示滚动条

	public:
		//获取滚动信息
		SCROLLINFO get_info(UINT mask) const
		{
			//准备数据
			SCROLLINFO si{};
			si.cbSize = sizeof(si);
			si.fMask = mask;

			//获取滚动信息
			utils_assert(GetScrollInfo(m_id.m_hwnd, m_id.m_flag, &si));

			//给出
			return si;
		}
		//设置滚动信息
		void set_info(const SCROLLINFO & si) const
		{
			//复制参数，方便修改
			auto _si = si;

			//修改标志
			utils::basic::bits::
				set_bits::inplace::by_bool<UINT,UINT>(
					_si.fMask, m_is_disable_no_scroll, SIF_DISABLENOSCROLL);

			//设置滚动信息
			SetScrollInfo(
				m_id.m_hwnd,
				m_id.m_flag,
				&_si,
				m_is_redraw ? TRUE : FALSE);

			//note: 似乎 SetScrollInfo 不该检查 GetLastError 的值(似乎即使执行成功，也返回“拒绝访
			//		问”，即使在调用SetScrollInfo前，调用了SetLastError(ERROR_SUCCESS) )
			//note: 似乎对于 SetScrollInfo 检查不了执行是否成功
		}	
		//设置滚动范围
		void set_range(const s8 nMin, const s8 nMax, const s8 nPage) const
		{
			//准备数据
			SCROLLINFO si{};
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE;
			utils::basic::store_in(si.nMin, nMin);
			utils::basic::store_in(si.nMax, nMax);
			utils::basic::store_in(si.nPage, nPage);

			//设置滚动信息
			set_info(si);
		}
		//设置滚动位置
		void set_position(const s8 nPos) const
		{
			//准备数据
			SCROLLINFO si{};
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			utils::basic::store_in(si.nPos, nPos);

			//设置滚动信息
			set_info(si);
		}
	};

	//处理 WM_VSCROLL/WM_HSCROLL 消息
	//single_step: 单步进
	//check_scroll_position: 传出计算出的新值，并获取被校正后的值，用此值来更新滚动条
	//is_disable_no_scroll: 当不需要滚动时，滚动条是否显示
	inline LRESULT meet_window_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		s8 single_step, std::function<void(int & new_value)> check_scroll_position,
		bool is_disable_no_scroll = true)
	{
		//简单检查
		utils_assert(uMsg == WM_VSCROLL || uMsg == WM_HSCROLL);
		utils_assert(single_step >= 1);

		//拆解参数
		const UINT code = LOWORD(wParam);	//通知码
		const HWND hCtl = (HWND)lParam;   // 0 表示内置条

		//滚动条
		ScrollBar scroll_bar;

		//绑定
		scroll_bar.m_id.set_by_window_message(hwnd, uMsg, lParam);

		//设置参数
		scroll_bar.m_is_disable_no_scroll = is_disable_no_scroll;

		//获取滚动信息
		auto si = scroll_bar.get_info(SIF_ALL);

		//处理通知
		switch(code)
		{
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			si.nPos = si.nTrackPos;
			break;

		case SB_ENDSCROLL:
			return 0;	//直接返回，无需写回

		case SB_LINEUP:   si.nPos -= single_step; break;
		case SB_LINEDOWN: si.nPos += single_step; break;
		case SB_PAGEUP:   si.nPos -= si.nPage; break;
		case SB_PAGEDOWN: si.nPos += si.nPage; break;
		case SB_TOP:      si.nPos = si.nMin; break;
		case SB_BOTTOM:   si.nPos = si.nMax - (std::max)(si.nPage,1u) + 1; break;

		default: utils_throw();
		}

		//给出滚动位置，并获取它被限制后的值
		check_scroll_position(si.nPos);

		//写回
		si.fMask = SIF_POS;
		scroll_bar.set_info(si);

		return 0;
	}
	//获得系统推荐的 垂直滚动条的宽度/水平滚动条的高度
	inline s8 get_system_hint_size()
	{
		//查询
		const int a = GetSystemMetrics(SM_CXVSCROLL);	//垂直滚动条的宽度
		const int b = GetSystemMetrics(SM_CYHSCROLL);	//水平滚动条的高度
		
		//简单检查
		utils_assert(a == b);

		//给出
		return a;
	}
}