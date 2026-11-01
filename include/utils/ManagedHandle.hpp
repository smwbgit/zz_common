#pragma once

#include <utils/assert.hpp>
#include <utils/optional.hpp>

namespace utils
{
	//能自动关闭的句柄
	//handle_T: 句柄的原生类型
	template<typename handle_T>
	class ManagedHandle
	{
	private:
		std::optional<handle_T> m_handle;
	protected:
		std::function<void(handle_T handle)> m_close_handler;
	public:
		ManagedHandle() = default;
		ManagedHandle(handle_T handle)
		{
			m_handle = handle;
		}
		ManagedHandle(std::optional<handle_T> handle)
		{
			m_handle = handle;
		}
		ManagedHandle(const ManagedHandle &) = delete;
		ManagedHandle(ManagedHandle && o)
		{
			m_handle = o.m_handle;
			o.m_handle.reset();
			m_close_handler = o.m_close_handler;
		}
		ManagedHandle & operator=(const ManagedHandle &) = delete;
		ManagedHandle & operator=(ManagedHandle && o)
		{
			//简单检查
			utils_assert(!m_handle.has_value());

			m_handle = o.m_handle;
			o.m_handle.reset();
			m_close_handler = o.m_close_handler;
			return *this;
		}
		void set(handle_T handle)
		{
			//简单检查
			utils_assert(!m_handle.has_value());

			m_handle = handle;
		}
		void close()
		{
			if (m_handle.has_value())
			{
				//简单检查
				utils_assert(m_close_handler);

				m_close_handler(m_handle.value());
				m_handle.reset();
			}
		}
		~ManagedHandle()
		{
			close();
		}
		handle_T value() const
		{
			utils_assert(m_handle.has_value());
			return m_handle.value();
		}
		operator handle_T() const
		{
			return value();
		}
	};
}