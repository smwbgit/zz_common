#pragma once

#include <vector>
#include <functional>
#include <utils/value/property/Property.hpp>

namespace utils::value::property
{
	//向前声明
	template<typename T>
	class ReadOnlyPropertyController;

	//只读属性
	//note: 非线程安全
	template<typename T>
	class ReadOnlyProperty
	{
	private:
		utils::value::property::Property<T> m_property;
	public:
		ReadOnlyProperty()
		{
		}
		explicit ReadOnlyProperty(const T & value)
			: m_property{ value }
		{
		}
	public:
		T get_value() const
		{
			//转发
			return m_property.get_value();
		}
		void add_listener(std::function<void()> listener)
		{
			//转发
			m_property.add_listener(listener);
		}
		void add_assertor(std::function<void(const T & new_value)> assertor)
		{
			//转发
			m_property.add_assertor(assertor);
		}
		void add_bind(ReadOnlyProperty & other)
		{
			//转发
			m_property.add_bind(other.m_property);
		}
		//特化
		ReadOnlyProperty & operator += (std::function<void()> listener)
		{
			//转发
			add_listener(listener);

			return *this;
		}
	private:
		friend class ReadOnlyPropertyController<T>;

		void set_value(const T & value)
		{
			//转发
			m_property.set_value(value);
		}
		void add_checker(std::function<void(T & new_value)> checker)
		{
			//转发
			m_property.add_checker(checker);
		}
	};

	//配套类：给 ReadOnlyProperty 赋值
	template<typename T>
	class ReadOnlyPropertyController
	{
	private:
		ReadOnlyProperty<T> & m_read_only_property;
	public:
		explicit ReadOnlyPropertyController(ReadOnlyProperty<T> & read_only_property)
			: m_read_only_property{ read_only_property }
		{
		}
		void set_value(const T & value)
		{
			//转发
			m_read_only_property.set_value(value);
		}
		void add_checker(std::function<void(T & new_value)> checker)
		{
			//转发
			m_read_only_property.add_checker(checker);
		}
	};
}