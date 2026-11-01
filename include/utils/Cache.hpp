#pragma once

#include <functional>
#include <optional>

namespace utils
{
	//缓存一个值
	template<typename T>
	class Cache
	{
	public:
		std::function<T()> m_get_update_value;
	private:
		std::optional<T> m_value;
	public:
		const T & get_value()
		{
			if(!m_value.has_value())
				update_value();
			return m_value.value();
		}
		void update_value()
		{
			m_value = m_get_update_value();
		}
	};
}