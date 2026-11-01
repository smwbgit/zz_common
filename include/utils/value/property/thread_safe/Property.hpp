#pragma once

#include <vector>
#include <functional>
#include <utils/thread/AtomicObject.hpp>

namespace utils::value::property::thread_safe
{
	//特化: utils::value::property::Property 的 线程安全 版
	template<typename T>
	class Property
	{
	private:
		utils::thread::AtomicObject<std::vector<std::function<void()>>> m_listeners;
		utils::thread::AtomicObject<T> m_value;
	public:
		//设置值
		void set_value(const T & value)
		{
			//先获取锁，保存值，释放锁，然后再发出通知
			//发出通知时，先加锁获取一份通知列表的副本，释放锁，然后再慢慢通知

			{
				//获取锁
				auto _m_value = m_value.lock();

				//已为此值
				if(*_m_value == value)
					return;

				//保存新值
				*_m_value = value;
			}

			//发出通知

			//获取通知列表的副本
			const auto listeners = *m_listeners.lock();

			//依次通知
			for(const auto & listener : listeners)
			{
				listener();		//这里特意不传值，让它们自己去查新值，来避免两个线程，同时发出通知时的竞争问题
			}
		}
		//获取值
		T get_value() const
		{
			//查询
			const auto value = *m_value.lock();

			//给出
			return value;
		}
		void add_listener(std::function<void()> listener)
		{
			//保存
			m_listeners.lock()->push_back(listener);
		}
		//特化
		Property & operator += (std::function<void()> listener)
		{
			//转发
			add_listener(listener);

			return *this;
		}
	};
}