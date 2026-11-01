#pragma once

#include <algorithm>
#include <utils/integer_type.hpp>

namespace utils
{
	class Range1D
	{
	public:
		s8 position{};
		s8 size{};

	public:
		Range1D() = default;
		Range1D(s8 position, s8 size)
			: position{ position }
			, size{ size }
		{
			//简单检查
			utils_assert_abort(size >= 0);
		}
	public:
		//是否包含
		bool is_include(s8 value) const
		{
			//简单检查
			utils_assert(size >= 0);

			return position <= value && value < position + size;
		}
		//是否有重叠区域
		bool is_overlap(Range1D other) const
		{
			//简单检查
			utils_assert(size >= 0);

			return
				this->is_include(other.position) ||
				other.is_include(this->position);
		}
		//把指定值限制在当前范围内
		void clamp_inplace(s8 & value) const
		{
			//简单检查
			utils_assert(size >= 0);

			value = std::clamp(value, position, position + size - 1);
		}
		//获取最后一个值
		s8 get_last_value() const
		{
			//简单检查
			utils_assert(size >= 1);

			return position + size - 1;
		}

		//中心对称变换
		//note: 把范围内的一个值，以范围的中心为对称轴，变到另一边去。
		s8 central_symmetry_transform(const s8 value) const
		{
			//简单检查
			utils_assert(is_include(value));
			utils_assert(size > 0);

			return position * 2 + size - value;
		}
	public:
		bool operator == (const Range1D &) const = default;
		bool operator != (const Range1D &) const = default;
	};
}