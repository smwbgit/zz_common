#pragma once

#include <utils/map.hpp>

namespace utils
{
	//双向map
	//note: 对于修改操作，提供直接函数，对于查询操作，直接返回const引用(正向和反向两种)
	template<typename T1, typename T2>
	class bidirectional_map
	{
	private:
		std::map<T1,T2> m_forward;
		std::map<T2,T1> m_backward;
	public:
		//获得正向map
		const std::map<T1,T2> & get_forward() const
		{
			return m_forward;
		}
		//获得反向map
		const std::map<T2,T1> & get_backward() const
		{
			return m_backward;
		}
		//插入一个不存在的
		void insert_not_exist(const T1 & key, const T2 & value)
		{
			utils::map(m_forward).insert_not_exist(key,value);
			utils::map(m_backward).insert_not_exist(value,key);
		}
	};
}