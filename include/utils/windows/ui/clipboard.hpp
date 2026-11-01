#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/text/encoding.hpp>

//剪贴板
namespace utils::windows::clipboard
{
	//获取剪贴板的文本数据
	//refer: https://blog.csdn.net/yoryky/article/details/109118588
	inline std::u8string get_text()
	{
		utils_assert(OpenClipboard(NULL));
    	utils_assert(IsClipboardFormatAvailable(CF_UNICODETEXT));

    	auto handle = GetClipboardData(CF_UNICODETEXT);
		utils_assert(handle);
    	auto _text =  GlobalLock(handle);
		utils_assert(_text);
		std::u16string text = (const char16_t*)_text;
    	GlobalUnlock(handle);
    	utils_assert(CloseClipboard());
		return utils::text::encoding::unicode::utf16_to_utf8(text);
	}
	//设置剪贴板的文本数据
	inline void set_text(const std::u8string & text)
	{
		const auto u16 = utils::text::encoding::unicode::utf8_to_utf16(text);
		utils_assert(OpenClipboard(NULL));

		//创建内存块
		auto handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (u16.length() + 1) * sizeof(char16_t));
		utils_assert(handle);
		auto buffer = GlobalLock(handle); 
		memcpy((uint8_t*)buffer, u16.c_str(), (u16.length() + 1) * sizeof(char16_t));
		GlobalUnlock(handle);

		EmptyClipboard();
		utils_assert(SetClipboardData(CF_UNICODETEXT, handle));

		utils_assert(CloseClipboard());
	}
}