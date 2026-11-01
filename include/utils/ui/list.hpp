#pragma once

#include <vector>
#include <memory>
#include <string>
#include "utils/function.hpp"
#include "utils/ui/scroll_bar.hpp"
#include "utils/windows/mfc/BEGIN_MESSAGE_MAP__IN_CLASS.hpp"
#include "utils/windows/mfc/warehouse.hpp"

//接口
namespace utils::ui::list
{
	class core_able
	{
	public:
		virtual void meet_double_click(uint64_t entry_index) = 0;
		virtual void meet_click(uint64_t entry_index) = 0;
		virtual void meet_right_click(uint64_t entry_index) = 0;
		virtual uint64_t get_entry_n() = 0;
		virtual bool get_text(_Out_ std::string & text, _In_ const uint64_t entry_index) = 0;
	};
	class modify_clothes_able
	{
	public:
		virtual void set_core(utils::ui::list::core_able* core) = 0;
		virtual void remove_all_entries() = 0;
		virtual void add_new_entry(const std::string & text) = 0;
	};
}
//core
namespace utils::ui::list
{
	template<typename T_clothes>
	class core : public  utils::ui::list::core_able
	{
	public:
		class s_entry
		{
		public:
			std::string text;
			utils::function<void()> meet_double_click;
			utils::function<void()> meet_click;
			utils::function<void()> meet_right_click;
		};
	public:
		T_clothes clothes;
		
	private:
		std::vector<std::unique_ptr<s_entry>> m_entries;

	public:
		core()
		{
			clothes.set_core(this);
		}
		void clear()
		{
			//清空数据
			m_entries.clear();

			//清空ui
			clothes.remove_all_entries();
		}
		s_entry & push_back(const std::string & text)
		{
			//设置数据
			auto entry = std::make_unique<s_entry>();
			{
				entry->text = text;
			}
			s_entry & _entry = *entry;
			m_entries.push_back(std::move(entry));

			//设置ui
			clothes.add_new_entry(text);

			//给出引用
			return _entry;
		}
		uint64_t get_entry_n() override final
		{
			return m_entries.size();
		}
		bool get_text(_Out_ std::string & text, _In_ const uint64_t entry_index) override final
		{
			if (entry_index >= m_entries.size())
				return false;

			text = m_entries[entry_index]->text;
			return true;
		}
		void meet_double_click(uint64_t entry_index) override final
		{
			//简单检查
			utils_assert(entry_index < m_entries.size());

			//转发
			m_entries[entry_index]->meet_double_click.call_if_valued();
		}
		void meet_click(uint64_t entry_index) override final
		{
			//简单检查
			utils_assert(entry_index < m_entries.size());

			//转发
			m_entries[entry_index]->meet_click.call_if_valued();
		}
		void meet_right_click(uint64_t entry_index) override final
		{
			//简单检查
			utils_assert(entry_index < m_entries.size());

			//转发
			m_entries[entry_index]->meet_right_click.call_if_valued();
		}
	};
}
//clothes
namespace utils::ui::list::clothes::windows
{
	class mfc : public CWnd, public utils::ui::list::modify_clothes_able
	{
	private:
		utils::ui::list::core_able* m_core{};
		utils::ui::scroll_bar::core::with_blank<utils::ui::scroll_bar::clothes::windows::mfc> m_ui_scroll_bar_v;
		uint64_t m_select_index{};
	public:
		void set_core(utils::ui::list::core_able* core) override final
		{
			m_core = core;
		}
		//清空
		void remove_all_entries() override final
		{
			//更新滚动条
			m_ui_scroll_bar_v.set_line_n(0);
			
			//更新ui
			Invalidate();
		}
		//添加一条
		void add_new_entry(const std::string & text) override final
		{
			//更新滚动条
			m_ui_scroll_bar_v.set_line_n(m_core->get_entry_n());

			//更新ui
			Invalidate();
		}
	public:
		//创建
		void Create(CWnd* parent)
		{
			utils_assert(__super::Create(NULL, NULL, WS_CHILD | WS_CLIPCHILDREN, RECT{0,0,1,1}, parent, 0));
		}
	private:
		BOOL OnEraseBkgnd(CDC* pDC)
		{
			return TRUE;
		}
		BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
		{
			return m_ui_scroll_bar_v.clothes.OnMouseWheel(nFlags, zDelta, pt);
		}
		void OnPaint()
		{
			//准备
			utils::windows::mfc::DoubleBufferDC mem_dc{this};

			//清空背景
			using namespace utils::windows::mfc::warehouse::import;
			{
				CRect rect{};
				GetClientRect(rect);
				mem_dc.FillSolidRect(rect, warehouse::color::get(warehouse::color::Id::List_Line_Background_NotSelect));
			}

			//设置字体
			mem_dc.SelectObject(warehouse::font::get(warehouse::font::Id::List));

			//背景透明
			mem_dc.SetBkMode(TRANSPARENT);
			
			uint64_t show_x{};
			uint64_t show_line_index{};
			auto show = [&](const std::string & text, bool is_highlight = false)
			{
				//该行矩形
				CRect rect{};
				{
					GetClientRect(&rect);

					rect.top = (int)(show_line_index * warehouse::height::get(warehouse::height::Id::List_Line));
					rect.bottom = rect.top + (LONG)warehouse::height::get(warehouse::height::Id::List_Line);
				}

				//显示背景
				{
					//设置颜色
					COLORREF color;
					if (is_highlight)
						color = warehouse::color::get(warehouse::color::Id::List_Line_Background_Select);
					else
						color = warehouse::color::get(warehouse::color::Id::List_Line_Background_NotSelect);

					mem_dc.FillSolidRect(rect, color);
				}

				//设置文字颜色
				if (is_highlight)
					mem_dc.SetTextColor(warehouse::color::get(warehouse::color::Id::List_Line_Text_Select));
				else
					mem_dc.SetTextColor(warehouse::color::get(warehouse::color::Id::List_Line_Text_NotSelect));

				//显示文字
				mem_dc.DrawText(text.c_str(), &rect, DT_VCENTER | DT_SINGLELINE);
			};

			auto begin = m_ui_scroll_bar_v.get_position();
			for (uint64_t i = 0; i < m_ui_scroll_bar_v.get_page_size(); i++)
			{
				//获得数据
				std::string text;
				if (false == m_core->get_text(text, begin + i))
					break;

				//不显示空行
				if (text.empty())
					continue;

				//设置起始位置
				show_x = 0;
				show_line_index = i;

				//显示
				show(text, begin + i == m_select_index);
			}
		}
		int OnCreate(LPCREATESTRUCT lpCreateStruct)
		{
			if (__super::OnCreate(lpCreateStruct) == -1)
				return -1;

			using namespace utils::windows::mfc::warehouse::import;

			//创建滚动条
			m_ui_scroll_bar_v.clothes.set_line_height(warehouse::height::get(warehouse::height::Id::List_Line));
			m_ui_scroll_bar_v.on_core_changed = [&]()
			{
				if (GetSafeHwnd())
					Invalidate();
			};
			m_ui_scroll_bar_v.clothes.Create(this);
			return 0;
		}
		void OnSize(UINT nType, int cx, int cy)
		{
			__super::OnSize(nType, cx, cy);

			//放置滚动条
			{
				CRect rect{};
				GetClientRect(&rect);
				m_ui_scroll_bar_v.clothes.MoveWindow(rect);
			}
		}
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point)
		{
			using namespace utils::windows::mfc::warehouse::import;

			//如果没有点击到，则返回
			uint64_t entry_index = point.y / warehouse::height::get(warehouse::height::Id::List_Line) + m_ui_scroll_bar_v.get_position();
			if (entry_index >= m_core->get_entry_n())
				return;

			//设为选中状态
			m_select_index = entry_index;

			//给出通知
			m_core->meet_double_click(entry_index);

			//刷新
			Invalidate();
		}
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point)
		{
			using namespace utils::windows::mfc::warehouse::import;

			//获取焦点
			SetFocus();

			//如果没有点击到，则返回
			uint64_t entry_index = point.y / warehouse::height::get(warehouse::height::Id::List_Line) + m_ui_scroll_bar_v.get_position();
			if (entry_index >= m_core->get_entry_n())
				return;

			//设为选中状态
			m_select_index = entry_index;

			//给出通知
			m_core->meet_click(entry_index);

			//刷新
			Invalidate();
		}
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point)
		{
			using namespace utils::windows::mfc::warehouse::import;

			//获取焦点
			SetFocus();

			//如果没有点击到，则返回
			uint64_t entry_index = point.y / warehouse::height::get(warehouse::height::Id::List_Line) + m_ui_scroll_bar_v.get_position();
			if (entry_index >= m_core->get_entry_n())
				return;

			//设为选中状态
			m_select_index = entry_index;

			//给出通知
			m_core->meet_right_click(entry_index);

			//刷新
			Invalidate();
		}
	protected:
		BEGIN_MESSAGE_MAP__IN_CLASS(mfc)
			ON_WM_CREATE()
			ON_WM_SIZE()
			ON_WM_PAINT()
			ON_WM_MOUSEWHEEL()
			ON_WM_ERASEBKGND()
			ON_WM_LBUTTONDOWN()
			ON_WM_RBUTTONDOWN()
			ON_WM_LBUTTONDBLCLK()
		END_MESSAGE_MAP__IN_CLASS()
	};
}
