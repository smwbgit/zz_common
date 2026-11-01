#pragma once

#include <functional>

namespace utils
{
	class CallWhenReturn
	{
	public:
		CallWhenReturn(std::function<void()> func)
			: m_func{func}
		{
		}
		~CallWhenReturn()
		{
			if (m_is_call)
				m_func();
		}
		void do_not_when_return()
		{
			m_is_call = false;
		}
	private:
		std::function<void()> m_func;
		bool m_is_call{true};
	};
}
