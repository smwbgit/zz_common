#pragma once

#include <stdint.h>
#include "utils/assert.hpp"

namespace utils
{
	//次数检查：析构时，检查是否到达指定次数
	class CountChecker
	{
	private:
		uint64_t m_count_min{};
		uint64_t m_count_max{};
		uint64_t m_count{};
	public:
		CountChecker(uint64_t expected_count)
			: m_count_min{expected_count}
			, m_count_max{expected_count}
		{
		}
		//count_min: 退出时的最小值
		//count_max: 退出时的最大值
		CountChecker(uint64_t count_min, uint64_t count_max)
			: m_count_min{count_min}
			, m_count_max{count_max}
		{
		}
		~CountChecker()
		{
			//简单检查
			utils_assert_abort(m_count <= m_count_max && m_count >= m_count_min);
		}
		//次数加1
		void count()
		{
			//简单检查
			utils_assert(m_count+1 <= m_count_max);

			m_count++;
		}
		//当前次数
		uint64_t get_count() const
		{
			return m_count;
		}
	};
}