#pragma once

#include <algorithm>
#include <utils/assert.hpp>

namespace utils::math
{
	//求取最大值最小值
	template<typename T>
	class s_maxmin_value
	{
	public:
		//喂值
		void meet_one_value(_In_ const T & value)
		{
			if (m_is_meeted_value == false)
			{
				m_is_meeted_value = true;
				m_max = m_min = value;
			}
			else
			{
				m_max = (std::max)(m_max, value);
				m_min = (std::min)(m_min, value);
			}
		}
		T get_max() const
		{
			//简单检查
			utils_assert(m_is_meeted_value);

			return m_max;
		}
		T get_min() const
		{
			//简单检查
			utils_assert(m_is_meeted_value);

			return m_min;
		}
		T get_size() const
		{
			//简单检查
			utils_assert(m_is_meeted_value);

			return m_max-m_min;
		}
		//特化
		T get_addr_size() const
		{
			return get_size() + 1;
		}

	private:
		bool m_is_meeted_value{};
		T m_max{};
		T m_min{};
	};
}