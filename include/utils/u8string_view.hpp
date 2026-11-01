#pragma once

#include <string>
#include <utils/assert.hpp>

namespace utils
{
	namespace inner
	{
		class u8string_view : public std::u8string_view
		{
		public:
			using std::u8string_view::u8string_view;
			using std::u8string_view::operator =;

			//比较
		public:
			//是否相同: 不区分大小写
			bool compare_is_same(std::u8string_view o, bool is_ignore_case = false) const
			{
				if (is_ignore_case)
				{
					return std::equal(this->begin(), this->end(), o.begin(), o.end(), [](char8_t a, char8_t b)
					{
						return toupper(a) == toupper(b);
					});
				}
				else
					return *this == o;
			}
		};
	}

	const inner::u8string_view & u8string_view(const std::u8string_view & s)
	{
		return (const inner::u8string_view &)s;
	}
	inner::u8string_view & u8string_view(std::u8string_view & s)
	{
		return (inner::u8string_view &)s;
	}
}