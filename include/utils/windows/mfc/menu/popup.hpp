#pragma once

#include "utils/function.hpp"

namespace utils::windows::mfc::menu
{
	//如果没有选择则返回0，否则返回选择项的id
	//modify_menu: (可选) 在菜单显示前给出修改机会。比如当菜单存在单选项时，通过这个函数选中当前选项
	//e.g.:
	//	switch(utils::windows::mfc::menu::popup(this, IDR_POPUP_XML))
	//	{
	//	case ID_XML_GRAB:
	//		break;
	//	}
	inline uint32_t popup(CWnd* parent, uint16_t menu_id, utils::function<void(CMenu* menu)> modify_menu = {})
	{
		CMenu menu;
		menu.LoadMenuA(menu_id);

		CMenu* submenu = menu.GetSubMenu(0);

		//修改菜单
		modify_menu.call_if_valued(submenu);

		//获得鼠标位置
		CPoint point{};
		GetCursorPos(&point);

		return (uint32_t) submenu->TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, point.x, point.y, parent);
	}
}