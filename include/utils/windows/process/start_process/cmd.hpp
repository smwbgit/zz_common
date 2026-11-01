#pragma once

#include <vector>
#include <utils/text/encoding.hpp>

namespace utils::windows::process::start_process::cmd
{
	//拼出完整命令
	inline void get_whole_cmd(std::u8string & cmd, const std::u8string & full_filename,
		const std::vector<std::u8string> & params)
	{
		cmd.clear();
		cmd += u8"\"" + full_filename + u8"\"";
		for(const auto & param : params)
			cmd += u8" \"" + param + u8"\"";
	}
	//特化
	inline void get_whole_cmd(std::wstring & cmd, const std::u8string & full_filename,
		const std::vector<std::u8string> & params)
	{
		std::u8string cmd_u8;
		get_whole_cmd(cmd_u8, full_filename, params);

		const auto cmd_u16 = utils::text::encoding::unicode::utf8_to_utf16(cmd_u8);
		cmd = (const std::wstring &)cmd_u16;
	}
}