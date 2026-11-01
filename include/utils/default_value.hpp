#pragma once

namespace utils
{
	//返回T{} 或者 void
	template<typename T>
	class default_value
	{
	public:
		T operator()()
		{
			return {};
		}
	};
	//特化
	template<>
	class default_value<void>
	{
	public:
		void operator()()
		{
		}
	};
}