#pragma once

#include <functional>
#include "utils/Index2D.hpp"

namespace utils::ui::layout
{
	//帮助在窗口创建过程中，计算每个控件的位置
	/* example:
		
		utils::ui::layout::Layout layout;

		auto add = [this, &layout](s_ui_line& ui_line, const std::string & desc, UINT id)
		{
			auto line = layout.new_line();
			ui_line.checkbox.Create(desc.c_str(), 0, line.new_column(100, 50), this, id);
			ui_line.text.Create("", 0, line.new_column(200, 50), this, id);
		};

		add(m_ui.money, "捡钱", ID_BUTTON_0 + 0);
		add(m_ui.F5, "F5", ID_BUTTON_0 + 1);
	*/
	class Layout
	{
	private:
		utils::Index2D m_pos;
		utils::Index2D m_size;
		std::function<void()> m_meet_finish;
	public:
		Layout(utils::Index2D pos = {})
			: m_pos{pos}
		{
		}
		~Layout()
		{
			if (m_meet_finish)
				m_meet_finish();
		}
		utils::Index2D pos() const
		{
			return m_pos;
		}
		utils::Index2D size() const
		{
			return m_size;
		}
		Layout new_line()
		{
			Layout layout(new_line_pos());
			layout.m_meet_finish = [this, &layout]()
			{
				new_line(layout.size());
			};
			return layout;
		}
		Layout new_column()
		{
			Layout layout(new_column_pos());
			layout.m_meet_finish = [this, &layout]()
			{
				new_column(layout.size());
			};
			return layout;
		}
		utils::Index2D::Range new_line(utils::Index2D line_size)
		{
			auto _pos = new_line_pos();
			m_size.j += line_size.j;
			if (line_size.i > m_size.i)
				m_size.i = line_size.i;
			return utils::Index2D::Range{_pos, line_size};
		}
		//特化
		utils::Index2D::Range new_line(int64_t w, int64_t h)
		{
			return new_line(utils::Index2D{w,h});
		}
		utils::Index2D::Range new_column(utils::Index2D column_size)
		{
			auto _pos = new_column_pos();
			m_size.i += column_size.i;
			if (column_size.j > m_size.j)
				m_size.j = column_size.j;
			return utils::Index2D::Range{_pos, column_size};
		}
		//特化
		utils::Index2D::Range new_column(int64_t w, int64_t h)
		{
			return new_column(utils::Index2D{w,h});
		}
	private:
		utils::Index2D new_line_pos() const
		{
			return {pos().i, (pos() + size()).j};
		}
		utils::Index2D new_column_pos() const
		{
			return {(pos() + size()).i, pos().j};
		}
	};
}