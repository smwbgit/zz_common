#pragma once

#include <utils/assert.hpp>

namespace utils::value::dirty_value
{
	//表示一个可能会脏的值
	//note: 非线程安全
	template<typename T>
	class DirtyValue
	{
	private:
		T m_value{};
		bool m_is_dirty{true};
	public:
		bool is_dirty() const
		{
			return m_is_dirty;
		}
		void set_dirty()
		{
			m_is_dirty = true;
		}
		void set_value(const T & value)
		{
			//简单检查
			utils_assert(m_is_dirty);

			//保存
			m_value = value;

			//更新数据
			m_is_dirty = false;
		}
		//特化
		void set_value(T && value)
		{
			//简单检查
			utils_assert(m_is_dirty);

			//保存
			m_value = std::move(value);

			//更新数据
			m_is_dirty = false;
		}
		const T & get_value() const
		{
			//简单检查
			utils_assert(!m_is_dirty);

			return m_value;
		}
	};
}