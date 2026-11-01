#pragma once

#include <mutex>

namespace utils::thread
{
	namespace inner::AtomicObject
	{
		template<typename T>
		class Locker
		{
		private:
			T* m_object{};
			std::mutex* m_mutex{};
			std::lock_guard<std::mutex> m_guard{};
		public:
			Locker(T* object, std::mutex* mutex)
				: m_object{object}
				, m_mutex{mutex}
				, m_guard{*m_mutex}
			{
			}
			T* operator->()
			{
				return m_object;
			}
			T & operator*()
			{
				return *m_object;
			}
		};
	}
	//把一个对象的每一个函数都变为原子的
	//note: 利用一个临时对象，来实现在调用函数前后，锁定对象
	template<typename T>
	class AtomicObject
	{
	private:
		T m_object{};
		mutable std::mutex m_mutex;
	public:
		//转发
		template<typename ... params_T>
		AtomicObject(params_T && ... params)
			: m_object{std::forward<params_T>(params)...}
		{
		}
		auto lock()
		{
			return inner::AtomicObject::Locker<T>(&m_object, &m_mutex);
		}
		//特化
		auto lock() const
		{
			return inner::AtomicObject::Locker<const T>(&m_object, &m_mutex);
		}
	};
}