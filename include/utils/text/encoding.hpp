#pragma once

#include <string>
#include <optional>

#ifdef utils_on_windows
#	include <windows.h>
#elif defined(utils_on_linux) ||  defined(utils_on_android)
#	include <iconv.h>
#else
#	error
#endif

#include <utils/assert.hpp>
#include <utils/integer_type.hpp>

//unicode 和 各字符集 之间的转换
namespace utils::text::encoding
{
	//字符集
	enum class Charset
	{
		gbk,
		unicode,
	};

#ifdef utils_on_windows
	namespace inner
	{
		//获取windows系统的同等的字符集名称
		inline UINT get_windows_charset(Charset charset)
		{
			switch(charset)
			{
			case Charset::unicode:
				//报错
				utils_throw();
			case Charset::gbk:
				//给出
				return 936;
			}

			//报错
			utils_throw();
		}
	}

	//简单检查
	static_assert(sizeof(wchar_t) == sizeof(char16_t));

	inline std::u16string to_unicode(const std::string& text, Charset charset)
	{
		//获取同等标识
		const auto _charset = inner::get_windows_charset(charset);

		auto size = MultiByteToWideChar(_charset, 0, text.c_str(), (int)text.length(), NULL, 0);
		std::u16string re; re.resize(size);
		utils_assert(size == MultiByteToWideChar(_charset, 0, text.c_str(), (int)text.length(),
								 (wchar_t*)re.c_str(), size));
		return re;
	}
	inline std::string from_unicode(const std::u16string& text, Charset charset)
	{
		//获取同等标识
		const auto _charset = inner::get_windows_charset(charset);

		const wchar_t* _text = (const wchar_t*)text.c_str();
		auto size = WideCharToMultiByte(_charset, 0, _text, (int)text.length(), NULL, 0, NULL, NULL);
		std::string re; re.resize(size);
		utils_assert(size == WideCharToMultiByte(_charset, 0, _text, (int)text.length(),
								 (char*)re.c_str(), size, NULL, NULL));
		return re;
	}
#elif defined(utils_on_linux) ||  defined(utils_on_android)
	namespace inner
	{
		//获取iconv库的同等的字符集名称
		inline const char* get_iconv_charset(Charset charset)
		{
			switch(charset)
			{
			case Charset::unicode:
				//报错
				utils_throw();
			case Charset::gbk:
				//给出
				return "GBK";
			}

			//报错
			utils_throw();
		}
	}

	inline std::u16string to_unicode(const std::string & text, Charset charset)
	{
		//ai生成后修改

		//获取同等标识
		const char* fromcode = inner::get_iconv_charset(charset);

		//打开转换器
		iconv_t cd = iconv_open("UTF-16LE", fromcode);

		//简单检查
		utils_assert(cd != (iconv_t)-1);

		//准备数据：输入数据
		//note: iconv 需要非 const 的 char*（接口历史原因）
		char* input = const_cast<char*>(text.c_str());
		const size_t input_size = text.length();
		size_t input_left = input_size;

		//准备数据：输出缓存区
		std::u16string output_text;
		output_text.resize(text.length());
		char* output = reinterpret_cast<char*>(output_text.data());
		const size_t output_size = output_text.size() * sizeof(char16_t);
		size_t output_left = output_size;

		//进行转换
		size_t ret = iconv(cd, &input, &input_left,
			&output, &output_left);

		//简单检查
		utils_assert(ret != (size_t)-1);
		utils_assert(input_left == 0);
		utils_assert(output_left <= output_size);
		utils_assert(output_left >= 0);

		//关闭转换器
		utils_assert(0 == iconv_close(cd));

		//计算实际转换长度
		const size_t converted = output_size - output_left;

		//简单检查
		utils_assert(converted % sizeof(char16_t) == 0);

		//计算实际转换的字符个数
		const size_t converted_n = converted / sizeof(char16_t);

		//截短到实际转换的位置
		output_text.resize(converted_n);

		//给出
		return output_text;
	}
	inline std::string from_unicode(const std::u16string & text, Charset charset)
	{
		//ai生成后修改

		//获取同等标识
		const char* tocode = inner::get_iconv_charset(charset);

		//打开转换器
		iconv_t cd = iconv_open(tocode, "UTF-16LE");

		//简单检查
		utils_assert(cd != (iconv_t)-1);

		//准备输入数据
		char* input = const_cast<char*>(reinterpret_cast<const char*>(text.c_str()));
		const size_t input_size = text.length() * sizeof(char16_t);
		size_t input_left = input_size;

		//准备输出缓冲区
		std::string output_text;
		output_text.resize(text.length() * 4);	//note: UTF-16 转 GBK/UTF-8，每字符最多输出 4 字节，这里保守分配
		char* output = output_text.data();
		const size_t output_size = output_text.size();
		size_t output_left = output_size;

		//进行转换
		size_t ret = iconv(cd, &input, &input_left,
			&output, &output_left);

		//简单检查
		utils_assert(ret != (size_t)-1);
		utils_assert(input_left == 0);
		utils_assert(output_left <= output_size);
		utils_assert(output_left >= 0);

		//关闭转换器
		utils_assert(0 == iconv_close(cd));

		//截短到实际转换的位置
		output_text.resize(output_size - output_left);

		//给出
		return output_text;
	}
#else
#	error
#endif

}

//utf8 utf16 utf32之间的转换
namespace utils::text::encoding
{
	inline std::u8string utf16_to_utf8(const std::u16string & u16)
	{
		//ai生成后整理

		std::u8string u8;
		for (::utils::u8 i = 0; i < u16.size(); i++)
		{
			const auto c = u16[i];
			if (c < 0x80) {
				u8.push_back(static_cast<char8_t>(c));
			} else if (c < 0x800) {
				u8.push_back(static_cast<char8_t>(0xC0 | (c >> 6)));
				u8.push_back(static_cast<char8_t>(0x80 | (c & 0x3F)));
			}  else if (c >= 0xD800 && c <= 0xDBFF) {
				//简单检查
				utils_assert(i + 1 < u16.size());

				const auto c2 = u16[i + 1];
				if (c2 >= 0xDC00 && c2 <= 0xDFFF) {
					uint32_t cp = 0x10000 + ((c - 0xD800) << 10) + (c2 - 0xDC00);
					u8.push_back(static_cast<char8_t>(0xF0 | (cp >> 18)));
					u8.push_back(static_cast<char8_t>(0x80 | ((cp >> 12) & 0x3F)));
					u8.push_back(static_cast<char8_t>(0x80 | ((cp >> 6) & 0x3F)));
					u8.push_back(static_cast<char8_t>(0x80 | (cp & 0x3F)));
					i++;
				} else {
					utils_throw();
				}
			} else if (c >= 0xDC00 && c <= 0xDFFF) {
				utils_throw();
			} else {
				u8.push_back(static_cast<char8_t>(0xE0 | (c >> 12)));
				u8.push_back(static_cast<char8_t>(0x80 | ((c >> 6) & 0x3F)));
				u8.push_back(static_cast<char8_t>(0x80 | (c & 0x3F)));
			}
		}
		return u8;
	}

#if 0
	//bug:
	//     "ed ac bf ed bf bd "会转为4个0xfffd
	//     ps. 上面好像不是bug，输入序列好像不是合法utf8
	inline std::u16string utf8_to_utf16(const std::u8string& u8)
	{
		auto size = MultiByteToWideChar(CP_UTF8, 0, (const char*)u8.c_str(), (int)u8.length(), NULL, 0);
		std::u16string re; re.resize(size);
		utils_assert(size == MultiByteToWideChar(CP_UTF8, 0, (const char*)u8.c_str(), (int)u8.length(),
			(wchar_t*)re.c_str(), size));
		return re;
	}
#endif

	inline std::u16string utf8_to_utf16(const std::u8string & u8, std::optional<char16_t> bad_char_default = {})
	{
		std::u16string u16;

		auto meet_bad_char = [&](uint64_t & i)
		{
			if(bad_char_default.has_value()) {
				u16.push_back(bad_char_default.value()); i++;
			}
			else utils_throw();
		};

		for(uint64_t i = 0; i < u8.size();)
		{
			uint32_t cp{};	//unicode码点
			uint8_t first = u8[i];
			uint64_t extra{};	//额外字节数
		
			// 1. 解析首字节，确定码点范围和后续字节数
			if (first < 0x80) {                      // 0xxxxxxx
				cp = first;
				extra = 0;
			} else if ((first & 0xE0) == 0xC0) {     // 110xxxxx
				cp = first & 0x1F;
				extra = 1;
			} else if ((first & 0xF0) == 0xE0) {     // 1110xxxx
				cp = first & 0x0F;
				extra = 2;
			} else if ((first & 0xF8) == 0xF0) {     // 11110xxx
				cp = first & 0x07;
				extra = 3;
			} else {
				meet_bad_char(i); continue;
			}
		
			//2. 读取后续字节

			if(i + extra >= u8.size()) {
				meet_bad_char(i); continue;
			}

			bool is_ok = true;
			for (size_t j = 0; j < extra; j++) {
				uint8_t b = u8[i+j+1];

				if((b & 0xC0) != 0x80) { is_ok = false; break; }

				cp = (cp << 6) | (b & 0x3F);
			}
			if(false == is_ok) { meet_bad_char(i); continue; }

			//3. 码点合法性检查
			if ((extra == 1 && cp < 0x80)   ||
				(extra == 2 && cp < 0x800)  ||
				(extra == 3 && cp < 0x10000)||
				(cp > 0x10FFFF)             ||
				(cp >= 0xD800 && cp <= 0xDFFF))
			{
				meet_bad_char(i); continue;
			}
		
			//4. UTF-16 编码
			if (cp < 0x10000) {                   // BMP
				u16.push_back(cp & 0xFFFF);
			} else {                              // 代理对
				cp -= 0x10000;
				uint16_t hi = 0xD800 + (cp >> 10);
				uint16_t lo = 0xDC00 + (cp & 0x3FF);
				u16.push_back(hi);
				u16.push_back(lo);
			}

			//更新状态
			i += extra+1;
		}

		return u16;
	}
}