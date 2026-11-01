#pragma once

#include <utils/integer_type.hpp>
#include <utils/Index2D.hpp>

namespace utils::ui::layout
{
	//外边距
	class OutsidePadding
	{
	public:
		s8 left{};
		s8 right{};
		s8 top{};
		s8 bottom{};

	public:
		//获得由外边距，而形成的大小
		utils::Index2D get_size() const
		{
			//给出
			return {left + right, top + bottom};
		}
	};
}