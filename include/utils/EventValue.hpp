#pragma once

#include "utils/function.hpp"

namespace utils
{
	//当值改变时能触发指定函数
	template<typename T>
	class EventValue
	{
	public:
		utils::function<void()> m_meet_value_changed;
	private:
		T m_value{};
	public:
		EventValue() = default;
		explicit EventValue(const T & value)
			: m_value{value}
		{
		}
		explicit EventValue(T && value)
			: m_value{std::move(value)}
		{
		}
		EventValue(const EventValue & value) = delete;
		void update(const T & value)
		{
			if (m_value != value)
			{
				m_value = value;
				m_meet_value_changed.call_if_valued();
			}
		}
		const T & get() const
		{
			return m_value;
		}
		T & get()
		{
			return m_value;
		}
		operator const T & () const
		{
			return m_value;
		}
	};
};