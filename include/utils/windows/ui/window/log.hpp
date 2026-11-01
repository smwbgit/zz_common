#pragma once

#include <windows.h>
#include <utils/text/format.hpp>
#include <utils/windows/ui/window/s_hwnd.hpp>
#include <utils/windows/process/get_information/single_one.hpp>
#include <utils/windows/process/integrity_level.hpp>

namespace utils::windows::ui::window::log
{
	inline std::u8string get_detail_text(HWND hwnd)
	{
		const auto title = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_title();
		const auto class_name = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_class_name();
		const auto is_visible = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).is_visible();
		const auto is_always_on_top = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).is_always_on_top();
		const auto position = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_position();
		const auto size = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_size();
		const auto pid = utils::windows::ui::window::s_hwnd::s_hwnd(hwnd).get_pid();
		const auto filename = utils::windows::process::get_information::single_one::get_full_filename(pid);
		const auto integrity_level = utils::windows::process::integrity_level::get_integrity_level(pid);

		const auto integrity_level_text = integrity_level ?
			integrity_level->get_text() : u8"<integrity_level.unknown>";

		std::u8string text;
		text += utils::text::format::format(u8"0x%llx %s \"%s\" %s", hwnd,
			title.c_str(), class_name.c_str(), is_visible ? u8"V" : u8"!V");
		text += utils::text::format::format(u8" %s {(%lld,%lld) (%lld,%lld)}",
			is_always_on_top ? u8"T" : u8"!T", position.i, position.j, size.i, size.j);
		text += utils::text::format::format(u8" %s",
			filename.value_or(u8"<exe.unknown>").c_str());
		text += utils::text::format::format(u8" %s",
			integrity_level_text.c_str());
		return text;
	}
	//特化
	inline std::u8string get_detail_text(std::optional<HWND> hwnd, const std::u8string & name)
	{
		return hwnd ?
			name + u8":" + get_detail_text(*hwnd) :
			u8"<" + name + u8".unknown>";
	}
}