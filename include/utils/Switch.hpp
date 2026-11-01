#pragma once

#include <functional>
#include <utils/assert.hpp>

namespace utils
{
	//表示一个开关
	class Switch
	{
	private:
		bool m_is_on{};
		bool m_is_check{};

	public:
		//is_on: 初始状态
		//is_check: 已经开/已经关时, 是否抛出异常
		Switch(bool is_on = false, bool is_check = true)
			: m_is_on{is_on}
			, m_is_check{is_check}
		{
		}
		//改变状态
		//is_on: 要变为的状态
		void turn_to(bool is_on)
		{
			//已经开/已经关
			if (is_on == m_is_on)
			{
				//简单检查
				utils_assert(!m_is_check);

				return;
			}
			
			//更新状态
			m_is_on = is_on;
		}
		//特化
		//turn_on:  打开时调用的函数
		//turn_off: 关闭时调用的函数
		void turn_to(bool is_on, std::function<void()> _turn_on,
			std::function<void()> _turn_off)
		{
			turn_to(is_on);

			if (m_is_on) _turn_on();
			else         _turn_off();
		}
		void turn_on()
		{
			turn_to(true);
		}
		void turn_off()
		{
			turn_to(false);
		}
		bool is_on() const
		{
			return m_is_on;
		}
	};
}