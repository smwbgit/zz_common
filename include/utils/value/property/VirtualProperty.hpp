#pragma once

#include <vector>
#include <functional>

namespace utils::value::property
{
	//向前声明
	template<typename T>
	class VirtualPropertyController;

	//代表一个需要实时算出，并不缓存的值
	//note: 非线程安全
	template<typename T>
	class VirtualProperty
	{
	private:
		std::function<void(T)> m_set_function;
		std::function<T()> m_get_function;
		std::vector<std::function<void()>> m_listeners;
	public:
		void set_value(T value)
		{
			//简单检查
			utils_assert(m_set_function);

			//转发
			m_set_function(value);
		}
		T get_value() const
		{
			//简单检查
			utils_assert(m_get_function);

			//转发
			return m_get_function();
		}
		void add_listener(std::function<void()> listener)
		{
			m_listeners.push_back(listener);
		}
	private:
		void set_set_function(std::function<void(T)> set_function)
		{
			//简单检查
			utils_assert(!m_set_function);

			//保存
			m_set_function = set_function;
		}
		void set_get_function(std::function<T()> get_function)
		{
			//简单检查
			utils_assert(!m_get_function);

			//保存
			m_get_function = get_function;
		}
		void changed() const
		{
			//发出通知
			const auto listeners = m_listeners;
			for(const auto & listener : listeners)
			{
				listener();
			}
		}
	private:
		friend class VirtualPropertyController<T>;
	};

	//负责访问 VirtualProperty 的 控制 函数
	//note: 非线程安全
	template<typename T>
	class VirtualPropertyController
	{
	private:
		VirtualProperty<T> & m_virtual_property;
	public:
		explicit VirtualPropertyController(VirtualProperty<T> & virtual_property)
			: m_virtual_property{virtual_property}
		{
		}
		void set_set_function(std::function<void(T)> set_function)
		{
			//转发
			m_virtual_property.set_set_function(set_function);
		}
		void set_get_function(std::function<T()> get_function)
		{
			//转发
			m_virtual_property.set_get_function(get_function);
		}
		void changed() const
		{
			//转发
			m_virtual_property.changed();
		}
	};
}