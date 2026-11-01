#pragma once

#include <compare>
#include <utils/assert.hpp>

namespace utils::pointer::inner
{
	//表示一个指针
	template<typename T, bool is_nullable, bool is_nullable_in_constructor = is_nullable>
	class Pointer
	{
		//简单检查: 不允许“可空，但在构造函数里不可空”这种情况
		static_assert(!(is_nullable && !is_nullable_in_constructor));

	private:
		T* m_pointer{};
	public:
		Pointer(T* pointer = nullptr)
			: m_pointer{ pointer }
		{
			if constexpr (!is_nullable_in_constructor)
			{
				//简单检查
				utils_assert(m_pointer);
			}
		}
		//支持 子到父 的自动转换
		template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		Pointer(const Pointer<U,is_nullable,is_nullable_in_constructor> & pointer)
			: Pointer{ pointer.get() }	//转发
		{
		}
	public:
		Pointer(const Pointer &) = default;
		Pointer & operator =(const Pointer &) = default;
	public:
		bool operator ==(const Pointer &) const = default;
	public:
		bool operator == (std::nullptr_t) const
		{
			if constexpr (!is_nullable)
			{
				//简单检查
				utils_assert(m_pointer);
			}

			return m_pointer == nullptr;
		}
		operator bool() const
		{
			if constexpr (!is_nullable)
			{
				//简单检查
				utils_assert(m_pointer);
			}

			return m_pointer;
		}
		operator T*() const
		{
			if constexpr (!is_nullable)
			{
				//简单检查
				utils_assert(m_pointer);
			}

			return m_pointer;
		}
		T* get() const
		{
			if constexpr (!is_nullable)
			{
				//简单检查
				utils_assert(m_pointer);
			}

			return m_pointer;
		}
		T* operator ->() const
		{
			//简单检查
			utils_assert(m_pointer);

			return m_pointer;
		}
		T& operator*() const
		{
			//简单检查
			utils_assert(m_pointer);

			return *m_pointer;
		}
	};
}