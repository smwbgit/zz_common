#pragma once

#include <afxwin.h>
#include <utils/Index2D.hpp>
#include <utils/text/namespace.hpp>
#include <utils/string.hpp>
#include <utils/va.hpp>
#include <utils/windows/mfc/begin_message_map.hpp>

//对原有类的包装，使其更容易使用
namespace utils::windows::mfc::buildin_control
{
	namespace inner
	{
		template<typename T>
		class Control
		{
		protected:
			T m_wrapped;
		public:
			T & get_wrapped()
			{
				return m_wrapped;
			}
			void set_text(const std::string & text)
			{
				m_wrapped.SetWindowTextA(text.c_str());
			}
			//特化
			void set_text(const char* format, ...)
			{
				utils_va_start(format);
				utils::string text; text.vformat(format, va);
				utils_va_end();

				return set_text(text);
			}
		protected:
			static CRect cast_to_CRect(const utils::Index2D::Range & rect)
			{
				return {POINT{(long)rect.pos().i,(long)rect.pos().j},SIZE{(long)rect.size().i,(long)rect.size().j}};
			}
		};

		namespace subclass
		{
			//添加消息反射
			class CButton : public ::CButton
			{
			public:
				std::function<void()> m_on_clicked;

			public:
				using ::CButton::CButton;

			private:
				void on_clicked()
				{
					if(m_on_clicked)
						m_on_clicked();
				}
			private:
				utils_begin_message_map(CButton)
					ON_CONTROL_REFLECT(BN_CLICKED,on_clicked)
				utils_end_message_map()
			};
		}
	}
	class CStatic : public inner::Control<::CStatic>
	{
	public:
		void create(const std::string & text, const utils::Index2D::Range & rect, CWnd* parent, DWORD dwStyle = WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, UINT nID = 0)
		{
			utils_assert(m_wrapped.Create(text.c_str(), dwStyle, cast_to_CRect(rect), parent, nID));
		}
		//特化: 显示图片
		void create_image(const utils::Index2D::Range & rect, CWnd* parent, DWORD dwStyle = WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE, UINT nID = 0)
		{
			create("", rect, parent, dwStyle, nID);
		}
		void set_bmp(HBITMAP hbitmap)
		{
			m_wrapped.SetBitmap(hbitmap);
		}
	};
	class CButton : public inner::Control<inner::subclass::CButton>
	{
	public:
		//按钮
		void create(const std::string & text, const utils::Index2D::Range & rect, CWnd* parent, std::function<void()> on_clicked, DWORD dwStyle = WS_CHILD | WS_VISIBLE)
		{
			utils_assert(m_wrapped.Create(text.c_str(), dwStyle, cast_to_CRect(rect), parent, 0));
			m_wrapped.m_on_clicked = on_clicked;
		}
	};
	//复选框
	class CCheckBox : public inner::Control<inner::subclass::CButton>
	{
	public:
		void create(const std::string & text, const utils::Index2D::Range & rect, CWnd* parent, bool enable, std::function<void()> on_clicked, DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_CHECKBOX)
		{
			//简单检查
			utils_assert(dwStyle & BS_CHECKBOX);
			
			utils_assert(m_wrapped.Create(text.c_str(), dwStyle, cast_to_CRect(rect), parent, 0));
			m_wrapped.m_on_clicked = on_clicked;

			set_check(enable);
		}
		void set_check(bool enable)
		{
			m_wrapped.SetCheck(enable ? BST_CHECKED : BST_UNCHECKED);
		}
	};
}
