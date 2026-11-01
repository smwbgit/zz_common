#pragma once

#include <windef.h>
#include <wingdi.h>
#include "utils/throw.hpp"

//颜色仓库
namespace utils::windows::mfc::warehouse::color
{
	//颜色id
	enum class Id
	{
		ScrollBar_Background,			//滚动条背景
		ScrollBar_Slider_MouseNotOn,	//滚动条滑块	鼠标不在其上
		ScrollBar_Slider_MouseOn,		//滚动条滑块	鼠标在其上
		List_Line_Background_NotSelect,	//列表框一行的背景	未选中时
		List_Line_Background_Select,	//列表框一行的背景	选中时
		List_Line_Text_Select,			//列表框一行的文字	未选中时
		List_Line_Text_NotSelect,		//列表框一行的文字	选中时
	};

	//获取一个颜色
	inline COLORREF get(_In_ const Id id)
	{
		switch(id)
		{
		case Id::ScrollBar_Background:
			return RGB(100,100,100);
		case Id::ScrollBar_Slider_MouseNotOn:
			return RGB(180,180,180);
		case Id::ScrollBar_Slider_MouseOn:
			return RGB(215,215,215);
		case Id::List_Line_Background_NotSelect:
			return RGB(40,40,40);
		case Id::List_Line_Background_Select:
			return RGB(214,214,214);
		case Id::List_Line_Text_NotSelect:
			return RGB(214,214,214);
		case Id::List_Line_Text_Select:
			return RGB(40,40,40);
		}
		utils_throw();
	}
}
