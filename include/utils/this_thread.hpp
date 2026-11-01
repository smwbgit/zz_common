#pragma once

#include <atomic>
#include <thread>

namespace utils::this_thread
{
	//休息一下
	//因为 std::this_thread::yield 似乎会导致当cpu空闲时cpu不休眠
	//所以 以sleep一会儿的方式，让cpu休眠
	inline void yield()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	//一直休息
	inline void yield_forever()
	{
		while(1)
			utils::this_thread::yield();
	}
	//等待出现指定值
	template<typename T>
	void wait(std::atomic<T> & value, const T & expected)
	{
		while(value.load() != expected)
			utils::this_thread::yield();
	}
	//特化
	inline void wait_true(std::atomic_bool & value)
	{
		wait(value, true);
	}
	//特化
	inline void wait_false(std::atomic_bool & value)
	{
		wait(value, false);
	}
	//等待准备好
	inline void wait_ready(std::atomic_bool & is_ready)
	{
		wait(is_ready, true);
	}
}
