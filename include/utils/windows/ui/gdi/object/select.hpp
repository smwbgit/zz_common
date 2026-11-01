#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows::ui::gdi::object::select
{
	namespace inner
	{
		class [[nodiscard]] s_auto_select_old
		{
		public:
			static s_auto_select_old create(HDC hdc, HGDIOBJ old_object, HGDIOBJ new_object)
			{
				return s_auto_select_old{hdc, old_object, new_object};
			}
		private:
			HDC m_hdc{};
			HGDIOBJ m_old_object{};
			HGDIOBJ m_new_object{};
		public:
			s_auto_select_old() = delete;
			s_auto_select_old(const s_auto_select_old &) = delete;
			s_auto_select_old(s_auto_select_old && o)
			{
				m_hdc = o.m_hdc;
				m_old_object = o.m_old_object;
				m_new_object = o.m_new_object;
				
				o.m_hdc = 0;
				o.m_old_object = 0;
				o.m_new_object = 0;
			}
			~s_auto_select_old()
			{
				if(m_old_object)
				{
					utils_assert_abort(SelectObject(m_hdc, m_old_object) == m_new_object);
				}
			}
		private:
			s_auto_select_old(HDC hdc, HGDIOBJ old_object, HGDIOBJ new_object)
				: m_hdc{ hdc }
				, m_old_object{ old_object }
				, m_new_object{ new_object }
			{
			}
		};
	}

	// 选择对象
	inline auto object(HDC hdc, HGDIOBJ new_object) -> inner::s_auto_select_old
	{
		// 简单检查
		utils_assert(new_object);

		// 选择
		auto old_object = ::SelectObject(hdc, new_object);

		// 简单检查
		utils_assert(old_object);

		// 给出清理动作
		return inner::s_auto_select_old::create(hdc, old_object, new_object);
	}
	// 选择字体
	inline auto font(HDC hdc, HFONT new_object)
	{
		//转发
		return object(hdc, new_object);
	}
	// 选择位图
	inline auto bitmap(HDC hdc, HBITMAP new_object)
	{
		//转发
		return object(hdc, new_object);
	}
}