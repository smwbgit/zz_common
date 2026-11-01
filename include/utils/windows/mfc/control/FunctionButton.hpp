#pragma once

#include "utils/windows/mfc/BEGIN_MESSAGE_MAP__IN_CLASS.hpp"
#include "utils/windows/mfc/DialogTemplate.hpp"
#include "utils/windows/mfc/ArgsDialog.hpp"
#include "utils/tuple.hpp"

namespace utils::windows::mfc::control
{
	//包含两个按钮，一个按钮负责执行指定的函数，一个按钮负责获取函数的参数
	//Func_T: 函数类型，返回值的类型必须为void      e.g.  void(int,int)
	template<typename T>
	class FunctionButton;
	//真正定义
	template<typename... Args>
	class FunctionButton<void(Args...)> : public CWnd
	{
	private:
		using func_t = void(Args...);
		using check_t = bool(Args...);

	public:
		FunctionButton() = default;
		~FunctionButton() override
		{
			DestroyWindow();
		}
	public:
		//创建
		void Create(const RECT& rect, CWnd* pParentWnd /*, UINT nID*/)
		{
			CWnd* pWnd = this;
			utils_assert(pWnd->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, pParentWnd, 0));
		}
		//设置检查参数的函数
		//如果参数合适，check()返回true。否则返回false
		void SetCheck(_In_ std::function<check_t> check)
		{
			//保存
			m_check = check;
		}
		//设置要调用的函数
		void SetFunction(_In_ std::function<func_t> func)
		{
			//保存
			m_func = func;
		}
	public:
		utils::windows::mfc::ArgsDialog<utils::remove_cvref_t<Args>...> m_dlg;

	private:
		std::function<check_t> m_check;
		std::function<func_t> m_func;
		std::tuple<utils::remove_cvref_t<Args>...> m_args;
	private:
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
		{
			if (__super::OnCreate(lpCreateStruct) == -1)
				return -1;

			//创建按钮
			DWORD dwStyle = BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE;
			utils_assert(m_button.Create("", dwStyle, CRect{1,1,1,1}, this, m_button_id));
			using namespace utils::windows::mfc::warehouse::import;
			warehouse::font::SetFont(m_button, warehouse::font::Id::Button);

			//增加"..."
			{
				CString text;
				GetWindowTextA(text);
				SetWindowTextA(text);
			}
			return 0;
		}
		void OnSize(UINT nType, int cx, int cy)
		{
			__super::OnSize(nType, cx, cy);

			//放置按钮
			{
				CRect rect{};
				GetClientRect(&rect);
				m_button.MoveWindow(rect);
			}
		}
		LRESULT OnSetText(WPARAM wParam, LPARAM lParam)
		{
			//todo 确定是unicode的还是ascii的

			//转发
			{
				//增加"..."
				CString text = (char*)lParam;
				text += "...";
				auto _lParam = (LPARAM)text.GetString();

				m_button.SendMessageA(WM_SETTEXT, wParam, _lParam);
			}

			//转默认处理
			return DefWindowProc(WM_SETTEXT, wParam, lParam);
		}
		void OnButtonClicked()
		{
			if (IDOK != m_dlg.DoModal())
				return;

			utils::apply(m_func, m_dlg.GetArgs());
//			utils::call_func(m_func, m_dlg.GetArgs());
		}
	private:
		CButton m_button;
		static const UINT m_button_id = 3;
	private:
		BEGIN_MESSAGE_MAP__IN_CLASS(FunctionButton)
			ON_MESSAGE(WM_SETTEXT, OnSetText)
			ON_WM_CREATE()
			ON_WM_SIZE()
			ON_BN_CLICKED(m_button_id, OnButtonClicked)
		END_MESSAGE_MAP__IN_CLASS()
	};
}
