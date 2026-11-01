#pragma once

#include <functional>

namespace utils
{
	//提供对一个函数只调用一次的功能，无论call多少次
	template<typename T>
	class CallOnce
	{
	public:
		CallOnce(std::function<T> func)
			: m_func{func}
		{
		}
		template<typename... Args>
		void call(Args && ... args)
		{
			if (m_is_called)
				return;
			return m_func(std::forward<Args>(args)...);
		}
	private:
		std::function<T> m_func;
		bool m_is_called = false;
	};
}