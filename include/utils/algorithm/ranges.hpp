#pragma once

#include <optional>
#include <functional>
#include <ranges>
#include <utils/assert.hpp>

namespace utils::ranges
{
	template<typename T>
	concept input_range = requires(T range){
		range.begin();
		range.end();
	};
}

namespace utils
{
	//容器里的元素的类型
	//note: 可能包含const
	template<typename T>
	using value_t = std::remove_reference_t<decltype(*((T*)0)->begin())>;
}

//使用utils::ranges表示容器
namespace utils::algorithm::ranges
{
#pragma region find
	//查找唯一一个
	template<std::ranges::input_range T_range>
	inline auto find_unique(T_range && range, std::function<bool(const std::remove_reference_t<decltype(*((T_range*)0)->begin())> &)> meet_one_element)
		-> utils::value_t<T_range> &
	{
		utils::value_t<T_range>* value{};
		for (auto& one : range)
		{
			if (meet_one_element(one))
			{
				//简单检查
				utils_assert(value == nullptr);

				value = &one;
			}
		}

		//简单检查
		utils_assert(value);

		return *value;
	}
#pragma endregion
}