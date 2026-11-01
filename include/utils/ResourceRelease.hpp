#pragma once

#include <functional>
#include <utils/assert.hpp>

namespace utils
{
	//资源释放
	template<typename Handle_T>
	class ResourceRelease
	{
	public:
		using release_func_t = std::function<void(Handle_T &)>;
	private:
		Handle_T m_handle{};
		release_func_t m_release_func;
	public:
		explicit ResourceRelease(release_func_t release_func)
			: m_release_func{release_func}
		{
			//简单检查
			utils_assert(m_release_func);
		}
		ResourceRelease(Handle_T handle, release_func_t release_func)
			: m_handle{handle}
			, m_release_func{release_func}
		{
			//简单检查
			utils_assert(m_release_func);
		}
		~ResourceRelease()
		{
			m_release_func(m_handle);
		}
		operator Handle_T&()
		{
			return m_handle;
		}
		operator const Handle_T&() const
		{
			return m_handle;
		}
		Handle_T & get()
		{
			return m_handle;
		}
		const Handle_T & get() const
		{
			return m_handle;
		}
		Handle_T & operator->()
		{
			return m_handle;
		}
		const Handle_T & operator->() const
		{
			return m_handle;
		}
	};
}