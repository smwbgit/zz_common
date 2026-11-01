#pragma once

#include <math.h>
#include <type_traits>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

namespace utils::basic
{
	//把浮点数存入整数或者浮点数类型里. 如果存入的是整数类型, 进行四舍五入
	//is_round_up_when_0_5: 当结果的小数部分正好是0.5时,是否进位
	template<bool is_round_up_when_0_5 = true, typename to_T, typename from_T>
	void store_this_floating_point(_Out_ to_T & to, _In_ const from_T & from)
	{
		//简单检查
		utils_assert(std::is_floating_point_v<from_T>);
		utils_assert(utils::is_arithmetic_v<to_T>);

		if (std::is_floating_point_v<to_T>)		//浮点数, 不需要四舍五入
			to = (to_T)from;
		else
		{
			//整数, 需要四舍五入

			//todo 检查浮点数是否能保存到整数里  e.g.  10000000000 -> short is not allowed

			//保存符号
			bool is_negative = from < 0;
			auto _from = (from_T)fabs(from);

			//四舍五入
			if (is_round_up_when_0_5)
				to = (to_T)(_from + (from_T)0.5);
			else
			{
				//算出整个部分的另一半的长度, 再算出要给出的长度

				auto whole = (to_T)(std::make_unsigned_t<to_T>)ceil(_from);
				auto rest = (to_T)(std::make_unsigned_t<to_T>)(whole - _from + (from_T)0.5);
				to = whole - rest;
			}

			//恢复符号
			if (is_negative)
				to = -to;
		}
	}
}