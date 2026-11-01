#pragma once

#include <vector>
#include <windows.h>
#include <utils/assert.hpp>
#include <utils/Index2D.hpp>
#include <utils/Range2D.hpp>
#include <utils/u8string.hpp>
#include <utils/color/Color.hpp>

namespace utils::windows::ui::window::s_hwnd
{
	//一个已存在的窗口
	struct s_hwnd
	{
		const HWND m_hwnd{};

		s_hwnd(HWND hwnd = {})
			: m_hwnd{hwnd}
		{
		}
#pragma region 位置和大小
		utils::Index2D get_position() const
		{
			RECT rect{};
			utils_assert(GetWindowRect(m_hwnd,&rect));
			return {rect.left, rect.top};
		}
		void set_position(utils::Index2D position) const
		{
			utils_assert(SetWindowPos(
				m_hwnd,
				NULL,
				position.i, position.j,
				0, 0,
				SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE));
		}
		//特化: 使用一个偏移,来设置新位置
		void set_position_by_offset(utils::Index2D offset) const
		{
			//当前位置
			const auto position = get_position();

			//新位置
			const auto new_position = position + offset;

			//设置位置
			set_position(new_position);
		}
		utils::Range2D get_rect() const
		{
			return { get_position(), get_size() };
		}
		void set_rect(const utils::Range2D rect) const
		{
			utils_assert(SetWindowPos(
				m_hwnd,
				NULL,
				rect.position.i, rect.position.j,
				rect.size.i, rect.size.j,
				SWP_NOZORDER));
		}
		utils::Index2D get_size() const
		{
			RECT rect{};
			utils_assert(GetWindowRect(m_hwnd, &rect));
			
			const auto w = rect.right - rect.left;
			const auto h = rect.bottom - rect.top;

			return {w, h};
		}
		void set_size(utils::Index2D size) const
		{
			utils_assert(SetWindowPos(
				m_hwnd,
				NULL,
				0, 0,
				size.i, size.j,
				SWP_NOZORDER | SWP_NOMOVE));
		}
		utils::Index2D get_client_size() const
		{
			RECT rect{};
			utils_assert(GetClientRect(m_hwnd, &rect));
			
			return {rect.right, rect.bottom};
		}
		//设置客户区大小
		void set_client_size(utils::Index2D size) const
		{
			DWORD dwStyle = GetWindowLongA(m_hwnd, GWL_STYLE); utils_assert(dwStyle);
			DWORD dwExStyle = GetWindowLongA(m_hwnd, GWL_EXSTYLE); utils_assert(dwExStyle);
			BOOL bMenu = [this]()
			{
				const auto re = GetClassLongPtrA(m_hwnd, GCLP_MENUNAME);
				if (re == 0)
					return FALSE;
				else
					utils_throw();	//todo 确认这里的情况
			}();
			RECT rect_pos{};
			utils_assert(::GetWindowRect(m_hwnd, &rect_pos));
			RECT rect{0, 0, (LONG)size.i, (LONG)size.j};
			utils_assert(::AdjustWindowRectEx(&rect, dwStyle, bMenu, dwExStyle));

			//大小已经是此值
			if (rect.right - rect.left == rect_pos.right - rect_pos.left &&
				rect.bottom - rect.top == rect_pos.bottom - rect_pos.top)
			{
				//不再继续
				return;
			}

			utils_assert(::MoveWindow(m_hwnd,
				rect_pos.left, rect_pos.top,
				rect.right - rect.left, rect.bottom - rect.top, TRUE));
		}
#pragma endregion
#pragma region 最大最小化
		//窗口是否处于最小化状态
		bool is_minimized() const
		{
			return IsIconic(m_hwnd);
		}
		//窗口是否处于最大化状态
		bool is_maximized() const
		{
			return IsZoomed(m_hwnd);
		}
		//把一个处于最小化状态的窗口，还原到之前的状态
		void restore_minimized_window() const
		{
			//简单检查
			utils_assert(is_minimized());

			ShowWindow(m_hwnd, SW_RESTORE);
		}
		//最小化指定窗口
		void minimize() const
		{
			if(!is_minimized())
				ShowWindow(m_hwnd, SW_MINIMIZE);
		}
		//最大化指定窗口
		void maximize() const
		{
			if(!is_maximized())
				ShowWindow(m_hwnd, SW_MAXIMIZE);
		}
		//在最小化和恢复之间，来回切换
		void toggle_between_minimize_and_restore() const
		{
			if(is_minimized())
				restore_minimized_window();
			else
				minimize();
		}
#pragma endregion
#pragma region 扩展样式
	private:
		DWORD get_style_core(int style_index) const
		{
			return (DWORD)get_window_long(style_index);
		}
		void modify_style_core(DWORD add_flags, DWORD remove_flags, int style_index) const
		{
			//不用修改
			if(add_flags == 0 && remove_flags == 0) return;

			//简单检查
			utils_assert((add_flags & remove_flags) == 0);

			//当前窗口样式
			DWORD current_style = get_style_core(style_index);
			
			//新的样式
			DWORD new_style = (current_style & ~remove_flags) | add_flags;
			
			//修改窗口样式
			set_window_long(style_index, new_style);
		}
		void set_exstyle_enable(bool is_enable, DWORD flag) const
		{
			if(is_enable)
				add_exstyle(flag);
			else
				remove_exstyle(flag);
		}
	public:
		bool is_always_on_top() const
		{
			return get_exstyle() & WS_EX_TOPMOST;
		}
		void set_double_buffer(bool is_enable) const
		{
			set_exstyle_enable(is_enable, WS_EX_COMPOSITED);
		}
		void set_visible_on_taskbar(bool is_visible) const
		{
			set_exstyle_enable(!is_visible, WS_EX_TOOLWINDOW);
		}
		void set_is_watermark(bool is_watermark) const
		{
			//设置
			set_exstyle_enable(is_watermark, WS_EX_TRANSPARENT | WS_EX_LAYERED);
		}
		DWORD get_exstyle() const
		{
			//转发
			return get_style_core(GWL_EXSTYLE);
		}
		void modify_exstyle(DWORD add_flags, DWORD remove_flags) const
		{
			//转发
			modify_style_core(add_flags, remove_flags, GWL_EXSTYLE);
		}
		void add_exstyle(DWORD flag) const
		{
			//转发
			return modify_exstyle(flag, 0);
		}
		void remove_exstyle(DWORD flag) const
		{
			//转发
			return modify_exstyle(0, flag);
		}
#pragma endregion
#pragma region 样式
	private:
		void set_style_enable(bool is_enable, DWORD flag) const
		{
			if(is_enable)
				add_style(flag);
			else
				remove_style(flag);
		}
	public:
		void set_clip_children(bool is_enable) const
		{
			set_style_enable(is_enable, WS_CLIPCHILDREN);
		}
		void set_clip_siblings(bool is_enable) const
		{
			set_style_enable(is_enable, WS_CLIPSIBLINGS);
		}
		void set_title_bar_exist(bool is_exist) const
		{
			set_style_enable(is_exist, WS_CAPTION);
		}
		void set_sysmenu_exist(bool is_exist) const
		{
			set_style_enable(is_exist, WS_SYSMENU);
		}
		void set_min_button_exist(bool is_exist) const
		{
			set_style_enable(is_exist, WS_MINIMIZEBOX);
		}
		void set_max_button_exist(bool is_exist) const
		{
			set_style_enable(is_exist, WS_MAXIMIZEBOX);
		}
		void set_thick_frame_exist(bool is_exist) const
		{
			set_style_enable(is_exist, WS_THICKFRAME);
		}
		DWORD get_style() const
		{
			//转发
			return get_style_core(GWL_STYLE);
		}
		void modify_style(DWORD add_flags, DWORD remove_flags) const
		{
			//转发
			modify_style_core(add_flags, remove_flags, GWL_STYLE);
		}
		void add_style(DWORD flag) const
		{
			//转发
			return modify_style(flag, 0);
		}
		void remove_style(DWORD flag) const
		{
			//转发
			return modify_style(0, flag);
		}
#pragma endregion
#pragma region z序
		//设置/取消设置 为顶层窗口
		void set_always_on_top(bool is_always_on_top) const
		{
			utils_assert(SetWindowPos(
				m_hwnd,
				is_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST,
				0, 0, 0, 0, // 位置和大小（使用标志保持原样）
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED));
		}
		//设置透明度
		//is_keep_color: 是否保持之前的透明色
		void set_watermark_alpha(BYTE alpha, bool is_keep_color = true) const
		{
			//保持之前透明色
			if(is_keep_color)
			{
				//获取透明色
				const auto color = get_watermark_color();

				//设置透明度和透明色
				set_watermark_alpha_and_color(alpha, color);
			}
			//否则
			else
			{
				//只设置透明度
				utils_assert(SetLayeredWindowAttributes(m_hwnd, 0, alpha, LWA_ALPHA));
			}
		}
		//设置透明度和透明色
		void set_watermark_alpha_and_color(BYTE alpha, utils::color::Color color) const
		{
			//简单转换
			const auto color_2 = RGB(color.r, color.g, color.b);

			//设置
			utils_assert(SetLayeredWindowAttributes(m_hwnd, color_2, alpha, LWA_ALPHA | LWA_COLORKEY));
		}
		//获取透明度
		BYTE get_watermark_alpha() const
		{
			//准备数据
			BYTE alpha{};

			//获取
			//todo 在未设置过透明度时,似乎,会获取失败,此时,考虑一下,是否直接返回255
			utils_assert(GetLayeredWindowAttributes(m_hwnd, nullptr, &alpha, nullptr));

			//给出
			return alpha;
		}
		//获取透明色
		utils::color::Color get_watermark_color() const
		{
			//准备数据
			COLORREF color{};

			//获取
			utils_assert(GetLayeredWindowAttributes(m_hwnd, &color, nullptr, nullptr));

			//简单转换
			const auto color_2 = utils::color::Color{GetRValue(color), GetGValue(color), GetBValue(color)};

			//给出
			return color_2;
		}
#pragma endregion
#pragma region 字体
		void set_font(HFONT font) const
		{
			SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)font, TRUE);
		}
		HFONT get_font() const
		{
			HFONT font = (HFONT)SendMessageW(m_hwnd, WM_GETFONT, 0, 0);
			utils_assert(font);
			return font;
		}
#pragma endregion
		//当前句柄是否有效
		bool is_valid() const
		{
			return IsWindow(m_hwnd) ? true : false;
		}
		//重绘窗口
		void invalidate_window() const
		{
			::InvalidateRect(m_hwnd, nullptr, FALSE);
		}
		void set_window_long(int nIndex, u8 value) const
		{
			// 准备数据
			std::optional<bool> was_topmost;

			// 当修改 扩展样式 时
			if (nIndex == GWL_EXSTYLE)
			{
				// 保存置顶状态
				was_topmost = is_always_on_top();
			}

			// 准备环境
			SetLastError(0);

			// 设置
			const auto result = SetWindowLongPtrW(m_hwnd, nIndex, value);

			// 简单检查
			utils_assert( result || ERROR_SUCCESS == GetLastError() );

			// 调用 SetWindowPos 使更改生效
			utils_assert(SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED));

			// 因刚才的,SetWindowPos,使用了,SWP_NOZORDER.而,似乎,windows,在修改 扩展样式 时,会丢失,置顶效果,所以,手动补偿回来
			// todo 确定这种,补偿,是否确实需要,以及,需要的话,该如何补偿

			// 补偿 置顶效果
			if (nIndex == GWL_EXSTYLE)
			{
				// 简单检查
				utils_assert(was_topmost);

				// 之前是置顶的
				if (*was_topmost)
				{
					// 进行补偿
					utils_assert(SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED));
				}
				else
				{
					// 进行补偿
					// note: 这个似乎也是需要的.
					utils_assert(SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED));
				}
			}
		}
		u8 get_window_long(int nIndex) const
		{
			//准备环境
			SetLastError(0);

			//获取
			const u8 value = GetWindowLongPtrW(m_hwnd, nIndex);

			//获取错误值
			const auto last_error = GetLastError();

			//简单检查
			utils_assert( value || ERROR_SUCCESS == last_error );

			//给出
			return value;
		}
		void set_window_extra_data(int nIndex, u8 value) const
		{
			//简单检查
			utils_assert(nIndex >= 0);

			set_window_long(nIndex, value);
		}
		//特化
		void set_window_extra_data(int nIndex, void* value) const
		{
			//转发
			return set_window_extra_data(nIndex, (u8)value);
		}
		u8 get_window_extra_data(int nIndex) const
		{
			//简单检查
			utils_assert(nIndex >= 0);

			return get_window_long(nIndex);
		}

		void set_text(const std::u8string & text) const
		{
			auto text_u16 = utils::text::encoding::utf8_to_utf16(text);
			utils_assert(SetWindowTextW(m_hwnd, (const wchar_t*)text_u16.c_str()));
		}
		std::u8string get_text() const
		{
			auto size = GetWindowTextLengthW(m_hwnd);
			if(size <= 0) return {};

			std::u16string text;
			text.resize(size);
			if(GetWindowTextW(m_hwnd, (wchar_t*)text.c_str(), size+1) <= 0)
				return {};

			return utils::text::encoding::utf16_to_utf8(text);
		}
		void set_title(const std::u8string & title) const
		{
			return set_text(title);
		}
		std::u8string get_title() const
		{
			return get_text();
		}
		bool is_visible() const
		{
			return IsWindowVisible(m_hwnd);
		}
		void set_visible(bool is_visible) const
		{
			if (is_visible)
				ShowWindow(m_hwnd, SW_SHOW);
			else
				ShowWindow(m_hwnd, SW_HIDE);
		}
		//变为前台窗口
		void set_foreground() const
		{
			utils_assert(SetForegroundWindow(m_hwnd));
		}
		//是否为前台窗口
		bool is_foreground() const
		{
			return GetForegroundWindow() == m_hwnd;
		}

		//获取输入焦点
		void get_focus() const
		{
			attach([&]()
			{
				utils_assert(SetFocus(m_hwnd) || GetLastError() == ERROR_SUCCESS);
			});
		}

		//对AttachThreadInput的简单包装
		void attach(std::function<void()> func) const
		{
			DWORD tid = GetWindowThreadProcessId(m_hwnd, NULL); utils_assert(tid);
			utils_assert(AttachThreadInput(GetCurrentThreadId(), tid, TRUE));
			func();
			utils_assert(AttachThreadInput(GetCurrentThreadId(), tid, FALSE));
		}
		
	private:
		static BOOL CALLBACK get_direct_children__EnumChildProc_callback(HWND hwnd,LPARAM lParam)
		{
			auto & hwnds = *(std::vector<HWND> *)lParam;
			hwnds.push_back(hwnd);
			return TRUE;
		}
	public:
		//获得所有直接子窗口, i.e.不递归搜索
		std::vector<s_hwnd> get_direct_children() const
		{
			std::vector<HWND> hwnds;
			EnumChildWindows(m_hwnd, get_direct_children__EnumChildProc_callback, (LPARAM)&hwnds);

			std::vector<s_hwnd> re;
			for(const auto hwnd : hwnds)
			{
				if(s_hwnd{hwnd}.get_parent() == m_hwnd)
				{
					re.push_back(hwnd);
				}
			}
			return re;
		}
		
		//获取类名
		std::u8string get_class_name() const
		{
			for(int size = 32; size < 10000; size *= 2)
			{
				std::u16string name;
				name.resize(size);
				auto writen_size = GetClassNameW(m_hwnd, (wchar_t*)name.c_str(), size);
				utils_assert(writen_size > 0 && writen_size < size);

				if(writen_size+1 == size)
					continue;

				name.resize(writen_size);
				return utils::text::encoding::utf16_to_utf8(name);
			}
			utils_throw();
		}
		//获取父窗口
		HWND get_parent() const
		{
			auto hwnd = GetParent(m_hwnd);
			utils_assert(hwnd);
			return hwnd;
		}
		DWORD get_pid() const
		{
			DWORD pid{};
			utils_assert(GetWindowThreadProcessId(m_hwnd, &pid));
			return pid;
		}
		DWORD get_tid() const
		{
			DWORD tid = GetWindowThreadProcessId(m_hwnd, NULL);
			utils_assert(tid);
			return tid;
		}
	};
}