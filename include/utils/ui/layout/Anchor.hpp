#pragma once

#include <vector>
#include <memory>
#include <string>
#include "utils/ui/Rect.hpp"
#include "utils/ui/Point.hpp"
#include "utils/ui/Size.hpp"
#include "utils/map.hpp"
#include "utils/string.hpp"

//ui布局
namespace utils::ui::layout
{
	namespace inner
	{
		//向前声明
		class s_anchor;

		//表示一个控件
		class s_rect
		{
		public:
			utils::ui::Rect m_rect;		//控件的位置
		private:
			const s_anchor* m_anchor{};
		public:
			s_rect() = default;
			//获得该控件的位置（经过锚点计算）
			utils::ui::Rect get_rect() const;
		private:
			friend class s_anchor;
			s_rect(const s_anchor* anchor)
			{
				m_anchor = anchor;
			}
		};
		//成组的控件
		class s_list
		{
		public:
			utils::ui::Point m_pt;	//第一个控件的位置
			utils::ui::Point m_step;//到下一个控件的步长
			utils::ui::Size m_size;	//一个控件的大小
		private:
			const s_anchor* m_anchor{};
		public:
			//获得成组排列时的位置（经过锚点计算）
			utils::ui::Rect get_rect(int index) const;
			//特化：成表格排列
			utils::ui::Rect get_rect(int i, int j) const;
			//特化
			utils::ui::Point get_pt(int index) const
			{
				return get_rect(index).top_left;
			}
			//特化
			utils::ui::Point get_pt(int i, int j) const
			{
				return get_rect(i, j).top_left;
			}
		private:
			friend class s_anchor;
			s_list(const s_anchor* anchor)
			{
				m_anchor = anchor;
			}
		};
		//锚点
		class s_anchor
		{
		public:
			utils::ui::Point m_pt;
		private:
			utils::map<utils::string, s_rect> m_rect;
			utils::map<utils::string, s_list> m_list;
			utils::map<utils::string, std::shared_ptr<s_anchor>> m_anchor;
			const s_anchor* m_parent{};
		public:
			s_rect& rect(const std::string& name)
			{
				if (m_rect.exist(name) == false)
					m_rect.insert(std::make_pair(name, s_rect(this)));

				return m_rect[name];
			}
			s_list& list(const std::string& name)
			{
				if (m_list.exist(name) == false)
					m_list.insert(std::make_pair(name, s_list(this)));

				return m_list[name];
			}
			s_anchor& anchor(const std::string& name)
			{
				if (m_anchor.exist(name) == false)
					m_anchor.insert(std::make_pair(name, new s_anchor(this)));

				return *(m_anchor[name]);
			}
		private:
			friend class s_list;
			friend class s_rect;
			s_anchor(const s_anchor* parent)
			{
				m_parent = parent;
			}
			utils::ui::Point get_pt() const
			{
				if (m_parent)
					return m_parent->get_pt() + m_pt;
				else
					return m_pt;
			}
		};

		//实现
		utils::ui::Rect s_rect::get_rect() const
		{
			return m_anchor->get_pt() + m_rect;
		}
		//实现
		utils::ui::Rect s_list::get_rect(int index) const
		{
			return utils::ui::Rect{m_anchor->get_pt() + m_pt + m_step * index, m_size};
		}
		//实现
		utils::ui::Rect s_list::get_rect(int i, int j) const
		{
			return utils::ui::Rect{m_anchor->get_pt() + m_pt + utils::ui::Point{m_step.x * i, m_step.y * j}, m_size};
		}
	}

	/*
		锚点

	Example:
		utils::ui::layout::anchor::Anchor m_ui;
		m_ui.anchor("0").m_pt = utils::ui::Point(14,8);
		m_ui.anchor("0").rect("frame").m_rect = utils::ui::Rect(0,0,473,230+12);
		m_ui.anchor("0").list("text").m_pt = utils::ui::Point(12,12+36);
		m_ui.anchor("0").list("text").m_step = utils::ui::Point(0,50);
		m_ui.anchor("0").list("text").m_size = utils::ui::Size(0,0);
		m_ui.anchor("0").list("edit").m_pt = utils::ui::Point(232,40);
		m_ui.anchor("0").list("edit").m_step = utils::ui::Point(0, 50);
		m_ui.anchor("0").list("text").m_size = utils::ui::Size(218,40);

		for (int i = 0; i < 4; i++)
		{
			edit[i].Create(m_ui.anchor("0").list("edit").get_rect(i), this, NULL);
		}
	*/
	using Anchor = inner::s_anchor;
}
