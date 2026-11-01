#pragma once

#include <windef.h>
#include "utils/throw.hpp"

//高度仓库
namespace utils::windows::mfc::warehouse::height
{
	//高度id
	enum class Id
	{
		List_Line,	//列表框一行的高度
		ScrollBar_BlankLine_Image,	//滚动条  图像时最下边的blank像素数
	};

	//获取一个高度
	inline uint64_t get(_In_ const Id id)
	{
		switch(id)
		{
		case Id::List_Line:
			return 28;
		case Id::ScrollBar_BlankLine_Image:
			return 44;
		}
		utils_throw();
	}
}
