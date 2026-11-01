#pragma once

#include <utils/windows/undocumented/process/by_it.hpp>
#include <utils/text/encoding.hpp>

namespace utils::windows::process::get_information::single_one
{
	inline DWORD get_ppid(DWORD pid)
	{
		//转发
		return utils::windows::undocumented::process::by_it::get_ppid(pid);
	}
	inline std::optional<std::u8string> get_full_filename(DWORD pid)
	{
		//打开进程
		utils::windows::handle::Handle hProcess;
		hProcess.set(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
		if(NULL == hProcess)
		{
			const auto error = GetLastError();
			if(ERROR_INVALID_PARAMETER == error)	//在pid==0时出现
				return {};
			if(ERROR_ACCESS_DENIED == error)
				return {};
			utils_throw();
		}

		//查询
		std::wstring buffer; buffer.resize(260);
		DWORD buffer_size = (DWORD)buffer.size();
		if(!QueryFullProcessImageNameW(hProcess, 0, (wchar_t*)buffer.c_str(), &buffer_size))
		{
			//简单检查
			utils_assert(
				ERROR_ACCESS_DENIED == GetLastError() ||	//权限不足
				ERROR_GEN_FAILURE == GetLastError());		//似乎出现在，进程已不存在时

			return {};
		}
		utils_assert(buffer_size + 4 < buffer.size());

		//整理数据
		std::wstring filename;
		filename.assign(buffer.begin(), buffer.begin()+buffer_size);
		const auto filename_u8 = utils::text::encoding::unicode::utf16_to_utf8((const std::u16string &)filename);

		//给出
		return filename_u8;
	}
}