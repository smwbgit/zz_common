#pragma once

#include <vector>
#include <memory>
#include <string>
#include "utils/function.hpp"
#include "utils/EventValue.hpp"
#include "utils/algorithm.hpp"
#include "utils/windows/mfc/BEGIN_MESSAGE_MAP__IN_CLASS.hpp"
#include "utils/windows/mfc/warehouse.hpp"

//接口
namespace utils::ui::scroll_bar
{
	class core_able
	{
	public:
		virtual uint64_t get_position() = 0;
		virtual uint64_t get_line_n() = 0;
		virtual uint64_t get_page_size() = 0;
		virtual uint64_t get_slider_n() = 0;
		virtual void set_page_size(_In_ const uint64_t page_size) = 0;
		virtual void offset_position(_In_ const int64_t offset) = 0;
	};
	class clothes_able
	{
	public:
		virtual void init(utils::ui::scroll_bar::core_able* core) = 0;
		virtual void core_changed() = 0;
	};
}
//core
namespace utils::ui::scroll_bar::core
{
	namespace inner
	{
		class slider
		{
		public:
			utils::EventValue<uint64_t> m_pos;
			utils::EventValue<uint64_t> m_n;

		public:
			//设置滑块位置
			void set_position(_In_ const uint64_t pos)
			{
				m_pos.update(fix_pos(pos, m_n));
			}
			//设置滑块位置
			void offset_position(_In_ const int64_t offset)
			{
				int64_t pos = (int64_t)m_pos.get() + offset;
				pos = utils::max<int64_t>(pos, 0);

				set_position(pos);
			}
			//设置n
			void set_n(_In_ const uint64_t n)
			{
				m_pos.update(fix_pos(m_pos, n));		//先改pos，以便向外通知时pos始终在n里
				m_n.update(n);
			}
		private:
			uint64_t fix_pos(_In_ const uint64_t pos, _In_ const uint64_t n)
			{
				if (n == 0)
					return 0;

				return utils::min<uint64_t>(pos, n - 1);
			}
		};
	}

	template<typename T_clothes>
	class normal : public utils::ui::scroll_bar::core_able
	{
	public:
		T_clothes clothes;
		utils::function<void()> on_core_changed;
	private:
		inner::slider m_slider;
		uint64_t m_line_n{};
		utils::EventValue<uint64_t> m_page_size{1};	//会被作为除数，不能为0

	public:
		normal()
		{
			clothes.init(this);

			m_slider.m_pos.m_meet_value_changed = [&]()
			{
				//更新状态
				clothes.core_changed();

				//发出通知
				on_core_changed.call_if_valued();
			};
			m_slider.m_n.m_meet_value_changed = [&]()
			{
				//更新状态
				clothes.core_changed();

				//发出通知
				on_core_changed.call_if_valued();
			};
			m_page_size.m_meet_value_changed = [&]()
			{
				//更新状态
				update_slider_n();
				clothes.core_changed();

				//发出通知
				on_core_changed.call_if_valued();
			};
		}
	public:
		//设置滑块位置
		void set_position(_In_ const uint64_t pos)
		{
			m_slider.set_position(pos);
		}
		//让滑块移动一些距离  i.e. pos += offset
		void offset_position(_In_ const int64_t offset) override final
		{
			m_slider.offset_position(offset);
		}
		//设置行数
		//note：pos 将会在[0, line_n - m_page_size]里
		void set_line_n(_In_ const uint64_t line_n)
		{
			//保存
			m_line_n = line_n;

			//设置滑动范围
			update_slider_n();
		}
		//设置一页的大小
		void set_page_size(_In_ const uint64_t page_size) override final
		{
			//最小为1
			m_page_size.update(utils::max<uint64_t>(page_size, 1));
		}
		//获得一页的大小
		uint64_t get_page_size() override final
		{
			return m_page_size;
		}
		//获得滑块位置
		uint64_t get_position() override final
		{
			return m_slider.m_pos;
		}
		uint64_t get_line_n() override final
		{
			return m_line_n;
		}
		uint64_t get_slider_n() override final
		{
			return m_slider.m_n;
		}
	private:
		//设置滑动范围
		void update_slider_n()
		{
			if (m_line_n == 0)
				m_slider.set_n(0);
			else if (m_line_n < m_page_size)
				m_slider.set_n(1);
			else
				m_slider.set_n(utils::max<uint64_t>(1, m_line_n - m_page_size + 1));
		}
	};

	template<typename T_clothes>
	class with_blank : public normal<T_clothes>
	{
	private:
		uint64_t m_blank_line_n{};

	public:
		with_blank(uint64_t blank_line_n = 1)
			: m_blank_line_n{blank_line_n}
		{
		}
		//设置可滑动范围
		//note：pos 将会在[0, line_n + m_blank_line_n)里
		void set_line_n(_In_ const uint64_t line_n)
		{
			__super::set_line_n(line_n + m_blank_line_n);
		}
	};
}
//clothes
namespace utils::ui::scroll_bar::clothes::windows
{
	class mfc : public CWnd, utils::ui::scroll_bar::clothes_able
	{
	private:
		utils::ui::scroll_bar::core_able* m_core{};

		//显示数据
		CRect m_rect{};
		utils::EventValue<CRect> m_slider_rect;
		bool m_is_bar_visible{};
		uint64_t m_line_height{1};

		//控制信息
		bool m_is_mouse_tracked{};
		utils::EventValue<bool> m_is_mouse_on_slider{};

	public:
		mfc()
		{
			m_slider_rect.m_meet_value_changed = [&]()
			{
				Invalidate();
			};
			m_is_mouse_on_slider.m_meet_value_changed = [&]()
			{
				Invalidate();
			};
		}
		~mfc() override
		{
			DestroyWindow();
		}
	public:
		void init(utils::ui::scroll_bar::core_able* core) override final
		{
			m_core = core;
		}
		void set_line_height(uint64_t line_height)
		{
			m_line_height = line_height;
			UpdatePageSize();
		}
		//创建
		void Create(CWnd* parent, bool is_visible = true)
		{
			DWORD style = WS_CHILD;
			if (is_visible)
				style |= WS_VISIBLE;
			utils_assert(__super::Create(NULL, NULL, style, CRect{0,0,1,1}, parent, 0));
		}
		void core_changed() override final
		{
			UpdateBarVisible();
			UpdateSliderRect();
		}
		//从父窗口转发来的鼠标滚轮消息
		BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
		{
			//设置超快浏览
			float scale{1};
			if (nFlags == MK_SHIFT)
			{
				//文档大小
				float doc_scale = 1.f * m_core->get_line_n() / m_core->get_page_size() / 6;

				//默认：滚动条的父窗口是显示部分
				CRect rect{};
				GetParent()->GetClientRect(&rect);

				auto _pt = pt;
				GetParent()->ScreenToClient(&_pt);

				//计算缩放
				scale = 1.f * (_pt.x - rect.left) / (rect.right - rect.left);
				scale *= scale;	//再小点
				scale *= scale;	//再小点
				scale = 1 + scale * doc_scale;
			}

			int64_t line_offset = -zDelta / WHEEL_DELTA;
			m_core->offset_position((int64_t)(line_offset * scale));

			return FALSE;
		}
	protected:
		afx_msg void OnPaint()
		{
			using namespace utils::windows::mfc::warehouse::import;

			//绘制背景
			utils::windows::mfc::DoubleBufferDC mem_dc{this};
			mem_dc.FillSolidRect(&m_rect, warehouse::color::get(warehouse::color::Id::ScrollBar_Background));

			//绘制滑块
			mem_dc.FillSolidRect(&m_slider_rect.get(), warehouse::color::get(m_is_mouse_on_slider ? warehouse::color::Id::ScrollBar_Slider_MouseOn : warehouse::color::Id::ScrollBar_Slider_MouseNotOn));
		}
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
		{
			if (__super::OnCreate(lpCreateStruct) == -1)
				return -1;

			return 0;
		}
		afx_msg void OnSize(UINT nType, int cx, int cy)
		{
			__super::OnSize(nType, cx, cy);

			using namespace utils::windows::mfc::warehouse::import;

			GetClientRect(m_rect);

			//更正到正确的宽度
			CRect rect = m_rect;
			{
				rect.left = rect.right - (LONG)warehouse::width::get(warehouse::width::Id::ScrollBar);
				if (rect.right - rect.left <= 1)	//宽度最小为1
					rect.left = rect.right - 1;
			}
			if (rect != m_rect)
			{
				MoveWindow(&rect);
				return;
			}

			UpdatePageSize();
			UpdateSliderRect();
		}
		afx_msg void OnMouseLeave()
		{
			//更新状态
			m_is_mouse_tracked = false;

			_OnMouseLeave();
		}
		afx_msg void OnMouseMove(UINT nFlags, CPoint point)
		{
			if (false == m_is_mouse_tracked)
			{
				//更新状态
				m_is_mouse_tracked = true;

				//通知系统发送鼠标离开消息
				utils::windows::api::message::track_mouse_leave(GetSafeHwnd());

				//手动模拟鼠标进入消息
				_OnMouseEnter();
			}

			//更新状态
			set_is_mouse_on_slider(m_slider_rect.get().PtInRect(point) ? true : false);

			__super::OnMouseMove(nFlags, point);
		}
	private:
		void UpdatePageSize()
		{
			//当窗口还未创建时，不计算
			if (GetSafeHwnd() == 0)
				return;

			//计算屏幕行数
			//默认：显示部分和滚动条一样高
			CRect rect{};
			GetClientRect(&rect);
			m_core->set_page_size(rect.Height() / m_line_height);
		}
		//更新滚动条的可见性
		void UpdateBarVisible()
		{
			//当窗口还未创建时，不计算
			if (GetSafeHwnd() == 0)
				return;

			m_is_bar_visible = m_core->get_line_n() > 0;

			ShowWindow(m_is_bar_visible ? SW_SHOW : SW_HIDE);
		}
		//更新滑块的矩形
		void UpdateSliderRect()
		{
			//当窗口还未创建时，不计算
			if (GetSafeHwnd() == 0)
				return;

			//计算高度
			uint64_t slider_height = 20;

			//计算y
			int64_t top{};
			if (m_core->get_slider_n() == 0)	//除零情况特殊处理
				top = 0;
			else
				utils::math::interpolate(top, 0, m_rect.Height() - slider_height, m_core->get_position(), 0, m_core->get_slider_n());

			//新的矩形
			auto slider_rect = m_rect;
			slider_rect.top = (LONG)top;
			slider_rect.bottom = (LONG)(top + slider_height);

			//保存
			m_slider_rect.update(slider_rect);
		}
		//鼠标进入
		void _OnMouseEnter()
		{
		}
		//鼠标离开
		void _OnMouseLeave()
		{
			//更新状态
			set_is_mouse_on_slider(false);
		}
		//设置鼠标是否在滑块上
		void set_is_mouse_on_slider(bool is_on)
		{
			m_is_mouse_on_slider.update(is_on);
		}

	private:
		BEGIN_MESSAGE_MAP__IN_CLASS(mfc)
			ON_WM_CREATE()
			ON_WM_SIZE()
			ON_WM_PAINT()
			ON_WM_MOUSEMOVE()
			ON_WM_MOUSELEAVE()
		END_MESSAGE_MAP__IN_CLASS()		
	};
}
