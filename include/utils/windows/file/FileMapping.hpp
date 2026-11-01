#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/windows/Handle.hpp>

namespace utils::windows::file
{
	//只读的文件映射
	class FileMapping
	{
	private:
		utils::windows::Handle m_hFile;
		utils::windows::Handle m_hFileMapping;
		const void* m_addr{};
	public:
		FileMapping(const std::u8string & filename)
		{
			auto filename_u16 = utils::text::encoding::unicode::utf8_to_utf16(filename);

			utils::windows::Handle hFile{CreateFileW(
				(const wchar_t*)filename_u16.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,                       // 默认安全性
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL                        // 默认模板文件
				),INVALID_HANDLE_VALUE};
    
			utils::windows::Handle hFileMapping{CreateFileMappingW(
				hFile,
				NULL,                     // 默认安全性
				PAGE_READONLY,
				0,                        // 高位大小
				0,						  // 低位大小
				NULL                      // 命名对象名
				),INVALID_HANDLE_VALUE};

			m_addr = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

			//简单检查
			utils_assert(m_addr);
    
			//保存
			m_hFile = std::move(hFile);
			m_hFileMapping = std::move(hFileMapping);
		}
		~FileMapping()
		{
			utils_assert_abort(UnmapViewOfFile(m_addr));
		}
	public:
		const void* addr() const
		{
			return m_addr;
		}
	};
}