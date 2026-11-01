#pragma once

#include <functional>
#include <vector>
#include <utils/assert.hpp>

namespace utils
{
	template<typename T>
	class functions
	{
	private:
		std::vector<std::function<T>> m_all;
	public:
		void operator += (std::function<T> one)
		{
			//简单检查
			utils_assert(one);

			m_all.push_back(one);
		}
		template<typename ... T_args>
		void call(T_args && ... args) const
		{
			for(const auto & one : m_all)
			{
				//简单检查
				utils_assert(one);

				one(std::forward<T_args>(args)...);
			}
		}
		//特化: 逆序调用
		template<typename ... T_args>
		void call_reverse(T_args && ... args) const
		{
			for(const auto & one : std::ranges::reverse_view(m_all))
			{
				//简单检查
				utils_assert(one);

				one(std::forward<T_args>(args)...);
			}
		}
	};
}