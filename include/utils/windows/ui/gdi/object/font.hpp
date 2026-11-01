#pragma once

#include <windows.h>
#include <utils/windows/ui/gdi/object/Object.hpp>
#include <utils/integer_type.hpp>
#include <utils/basic/store_this_integer.hpp>

namespace utils::windows::ui::gdi::object::font
{
	// 字体
	using Font = utils::windows::ui::gdi::object::Object<HFONT>;

	// 字体详细信息
	// note: 对 LOGFONTW 的简单包装
	// todo: 把 LOGFONTW 中,剩余的成员,也挪过来
	struct FontParam
	{
		std::u8string name = u8"微软雅黑";	//名字
		u8 size = 20;	//大小
		u8 weight = FW_NORMAL;	//字体粗细
		bool italic {};		//斜体
		bool underline {};	//下划线
		bool strike_out {};	//删除线
		u8 char_set = DEFAULT_CHARSET;
		u8 quality = CLEARTYPE_QUALITY;	// 清晰度
	};

	namespace inner
	{
		// 简单转换
		FontParam to_param(const LOGFONTW & lf)
		{
			FontParam param {};

			//todo 当大于0时,取 param.size 为 lf.lfHeight - 行距
			utils_assert(lf.lfHeight < 0);// 负值表示字符高度

			utils::basic::store_in(param.size, - lf.lfHeight);
			utils::basic::store_in(param.weight, lf.lfWeight);
			param.italic = lf.lfItalic ? true : false;
			param.underline = lf.lfUnderline ? true : false;
			param.strike_out = lf.lfStrikeOut ? true : false;
			utils::basic::store_in(param.char_set, lf.lfCharSet);
			utils::basic::store_in(param.quality, lf.lfQuality);
			{
				// 准备数据
				wchar_t lfFaceName[LF_FACESIZE + 1] {};

				// 复制
				memcpy(lfFaceName, lf.lfFaceName, LF_FACESIZE*2);
				
				// 复制
				std::u16string name_u16 = (const char16_t*)lfFaceName;

				// 简单转换
				param.name = utils::text::encoding::utf16_to_utf8(name_u16);
			}

			// 给出
			return param;
		}
		
		// 简单转换
		LOGFONTW from_param(const FontParam & param)
		{
			LOGFONTW lf {};
			utils::basic::store_in(lf.lfHeight, param.size);
			lf.lfHeight = - lf.lfHeight;	// 负值表示字符高度
			utils::basic::store_in(lf.lfWeight, param.weight);
			lf.lfItalic = param.italic ? TRUE : FALSE;
			lf.lfUnderline = param.underline ? TRUE : FALSE;
			lf.lfStrikeOut = param.strike_out ? TRUE : FALSE;
			utils::basic::store_in(lf.lfCharSet, param.char_set);
			utils::basic::store_in(lf.lfQuality, param.quality);
			{
				// 准备数据
				const auto name_u16 = utils::text::encoding::utf8_to_utf16(param.name);

				// 简单检查
				utils_assert(name_u16.length() < LF_FACESIZE);

				// 复制
				memcpy(lf.lfFaceName, name_u16.c_str(), name_u16.length()*2);
			}

			// 给出
			return lf;
		}
	}

	// 创建字体
	inline Font create_font(std::function<void(FontParam & param)> change_param)
	{
		// 准备数据
		FontParam param;

		// 调整参数
		change_param(param);

		// 创建字体
		HFONT hFont = [&]()
		{
			// 简单检查
			utils_assert(!param.name.empty());

			// 简单转换
			const LOGFONTW lf = inner::from_param(param);
    
			// 创建字体
			HFONT hFont = CreateFontIndirectW(&lf);

			// 简单检查
			utils_assert(hFont);

			// 给出
			return hFont;
		}();

		// 给出
		return Font{hFont};
	}
	// 特化
	inline Font create_font(const std::u8string & font_name, uint64_t font_size)
	{
		// 转发
		return create_font([&](auto & param)
		{
			param.name = font_name;
			param.size = font_size;
		});
	}
	//获取字体的详细信息
	inline FontParam get_info(const Font & font)
	{
		// 获取 LOGFONTW
		LOGFONTW lf{};
		utils_assert(GetObjectW(font, sizeof(lf), &lf) != 0);

		// 简单转换
		const FontParam param = inner::to_param(lf);

		return param;
	}
	// 把 带内间距的字体大小 ,转为,不带它的字体大小
	// cell_size: 带内间距的字体大小
	// change_param: 指定除 带内间距的字体大小 外的,其他参数
	inline u8 get_font_size_by_cell_size(const u8 cell_size, std::function<void(FontParam & param)> change_param)
	{
		// 核心逻辑,就是,不断试探

		// 从给定值,开始试探
		u8 font_size = cell_size;

		// 对于每个字体大小
		for( ; font_size >= 1; font_size--)
		{
			// 创建字体
			auto font = utils::windows::ui::gdi::object::font::create_font([change_param, font_size](auto & param)
			{
				// 其他参数
				change_param(param);

				// 字体大小
				param.size = font_size;
			});

			// 获取桌面dc
			const auto hdc = GetDC(0);

			// 使用字体
			font.select_me(hdc);

			// 获取内间距
			TEXTMETRICW tm{};
			utils_assert(GetTextMetricsW(hdc, &tm));

			// 保存: 带内间距的字符大小
			const auto size = tm.tmHeight;

			// 取消使用字体
			font.select_old(hdc);

			// 释放dc
			utils_assert(ReleaseDC(0, hdc));

			// 正是要找的那个
			if(size == cell_size)
			{
				//给出
				return font_size;
			}
		}

		// 没有找到
		utils_throw();
	}
}