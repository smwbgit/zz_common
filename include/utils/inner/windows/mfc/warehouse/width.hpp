#pragma once

#include <windef.h>
#include "utils/throw.hpp"

//宽度仓库
namespace utils::windows::mfc::warehouse::width
{
	//宽度id
	enum class Id
	{
		ScrollBar,	//滚动条的宽度
	};

	//获取一个宽度
	inline uint64_t get(_In_ const Id id)
	{
		switch(id)
		{
		case Id::ScrollBar:
			return 20;
		}
		utils_throw();
	}
}
