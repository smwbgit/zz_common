#pragma once

#include <utils/assert.hpp>

namespace utils
{
	//保证调用finish()之后不再调用
	template<typename T>
	class MarkedLife	//marked by finish() :)
	{
	private:
		bool m_is_finish = false;
		T m_object{};
	public:
		template<typename ...T>
		MarkedLife(T&&... args)
			: m_object(std::forward<T>(args)...)
		{
		}
		//完成。此后就不可以再调用任何函数
		auto finish()
		{
			//简单检查
			utils_assert(m_is_finish == false);

			m_is_finish = true;

			//转发
			return m_object.finish();
		}
		//执行操作
		T* operator -> ()
		{
			//简单检查
			utils_assert(m_is_finish == false);

			return &m_object;
		}
	};
}