#pragma once

#include <memory>
#include <windef.h>
#include <afxwin.h>
#include "utils/assert.hpp"
#include "utils/windows/api/handle.hpp"
#include "utils/windows/api/dpi.hpp"

//字体仓库
namespace utils::windows::mfc::warehouse::font
{
	namespace inner
	{
		//note: 会根据当前的dpi自动调整字体大小
		inline std::unique_ptr<CFont> make_font(int nHeight, LPCTSTR lpszFaceName)
		{
			int _nHeight{};
			utils::windows::api::dpi::get_scaled_size(_nHeight, nHeight);

			auto font = std::make_unique<CFont>();
			utils_assert(font->CreateFontA(
				_nHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				lpszFaceName));
			return font;
		}
	}

	//字体id
	enum class Id
	{
		Button,				//按钮上的文字
		Edit,				//CEdit控件里的文本
		Static_EditName,	//在CEdit之前，表示名字的CStatic控件
		Static_EditDesc,	//在CEdit之后，进行描述的CStatic控件
		List,				//列表框上的文字
		ComboBox,			//下拉列表的文字
//		Button_CheckBox_OnlyCheckBox,	//没有文字的复选框, 主要控制复选框的大小
	};

	//获取一个字体
	inline CFont* get(_In_ const Id id)
	{
		switch(id)
		{
		case Id::Button:
		{
			static std::unique_ptr<CFont> font = inner::make_font(16, "新宋体");
			return font.get();
		}
		case Id::Edit:
		{
			static std::unique_ptr<CFont> font = inner::make_font(16, "新宋体");
			return font.get();
		}
//		case Id::Button_CheckBox_OnlyCheckBox:
//		{
//			static std::unique_ptr<CFont> font = inner::make_font(6, "新宋体");
//			return font.get();
//		}
		case Id::Static_EditName:
		{
			static std::unique_ptr<CFont> font = inner::make_font(14, "黑体");
			return font.get();
		}
		case Id::Static_EditDesc:
		{
			static std::unique_ptr<CFont> font = inner::make_font(-12, "幼圆");
			return font.get();
		}
		case Id::List:
		{
			static std::unique_ptr<CFont> font = inner::make_font(14, "黑体");
			return font.get();
		}
		case Id::ComboBox:
		{
			static std::unique_ptr<CFont> font = inner::make_font(16, "新宋体");
			return font.get();
		}
		}
		utils_throw();
	}

	//便利函数
	template<typename T>
	void SetFont(_In_ T & control, _In_ const Id id)
	{
		control.SetFont(get(id));
	}
}