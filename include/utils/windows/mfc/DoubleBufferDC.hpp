#pragma once

#include <memory>
#include "utils/mfc/DC.hpp"

namespace utils::mfc
{
	//提供双缓冲功能
	//usage:
	//  void ShowCtrl::OnPaint()
	//  {
	//  	// 采用双缓存，防止闪烁
	//  	utils::mfc::DoubleBufferDC mdc(this);
	// 
	//  	mdc.FillSolidRect(&rc, bkgnd);
	//  	mdc.SelectObject(font);
	//  	mdc.SetBkMode(TRANSPARENT);
	//  	mdc.SetTextColor(RGB(255, 255, 255));
	//  	mdc.DrawText(text, -1, &rc, style);
	//		...
	//  
	//  	// 析构时使绘制生效
	//  }
	class DoubleBufferDC : public utils::mfc::DC
	{
	public:
		DoubleBufferDC(CWnd* wnd)
		{
			//创建窗口DC
			m_created_dc.reset(new CPaintDC{wnd});

			_init(wnd, m_created_dc.get());
		}
		DoubleBufferDC(CWnd* wnd, CDC* dc)
		{
			_init(wnd, dc);
		}
		~DoubleBufferDC()
		{
			// 将内存DC绘制到设备DC上
			m_dc->BitBlt(m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), this, 0, 0, SRCCOPY);
		}
	private:
		void _init(CWnd* wnd, CDC* dc)
		{
			m_dc = dc;

			// 创建内存DC
			wnd->GetClientRect(&m_rc);
			CreateCompatibleDC(m_dc);
			m_bmp.CreateCompatibleBitmap(m_dc, m_rc.Width(), m_rc.Height());
			SelectObject(m_bmp);
		}

		std::unique_ptr<CPaintDC> m_created_dc;
		CDC* m_dc{};
		CRect m_rc;
		CBitmap m_bmp;
	};
}

