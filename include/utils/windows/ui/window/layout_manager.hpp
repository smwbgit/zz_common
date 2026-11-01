#pragma once

#include <windows.h>
#include <vector>
#include <utils/Index2D.hpp>
#include <utils/windows/ui/window/s_hwnd.hpp>

namespace utils::windows::ui::window::layout_manager
{
	class BasicLayout
	{
	protected:
		using s_hwnd = utils::windows::ui::window::s_hwnd::s_hwnd;
	public:
		virtual ~BasicLayout()
		{
		}
		virtual void position_controls() = 0;
	};

	//垂直布局
	class VLayout : public BasicLayout
	{
	public:
		HWND m_parent{};
		std::vector<HWND> m_children;
		uint64_t m_padding{};
		utils::Index2D m_position;
	public:
		void position_controls() override
		{
			auto position = m_position;
			for(const auto i : utils::iterator_index(m_children))
			{
				const auto child = m_children[i];

				s_hwnd(child).set_position(position);

				const auto size = s_hwnd(child).get_size();

				position.j += size.j + m_padding;
			}
		}
	};

	//垂直布局, 靠右，在父窗口中
	class VLayout_RightAlign : public BasicLayout
	{
	public:
		HWND m_parent{};
		std::vector<HWND> m_children;
		uint64_t m_padding{};
		utils::Index2D m_position;
	public:
		void position_controls() override
		{
			auto parent_client_size = s_hwnd(m_parent).get_client_size();
			auto position = utils::Index2D{parent_client_size.i, 0} + m_position;
			for(const auto i : utils::iterator_index(m_children))
			{
				const auto child = m_children[i];

				s_hwnd(child).set_position(position);

				const auto size = s_hwnd(child).get_size();

				position.j += size.j + m_padding;
			}
		}
	};
	
}