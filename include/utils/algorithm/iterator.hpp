#pragma once

#include <optional>
#include <functional>

//算法, 用于迭代器的
namespace utils::algorithm::iterator
{
#pragma region is_exist
	//是否存在指定元素
	template<typename T_iterator>
	inline bool is_exist(const T_iterator & iterator, std::function<bool(const typename T_iterator::value_type&)> meet_one_element)
	{
		for (const auto& one : iterator)
		{
			if (meet_one_element(one))
				return true;
		}
		return false;
	}
#pragma endregion
#pragma region find
	//查找第一个
	//note: 在迭代器给定的一系列值里，通过谓词查找
	//example：
	/*
		auto pos_iterator = utils::iterator::index_2D::iterator_center(...);
		auto pos = utils::find_first(pos_iterator, [...](const auto & pos)
		{
			if (is_same(... , pos))
				return true;
			return false;
		});
		if(pos.has_value())
		{
			...
		}
		else
		{
			...
		}
	*/
	template<typename T_iterator>
	inline std::optional<typename T_iterator::value_type> find_first(const T_iterator & iterator, std::function<bool(const typename T_iterator::value_type&)> meet_one_element)
	{
		for (const auto& one : iterator)
		{
			if (meet_one_element(one))
				return {one};
		}
		return {};
	}
	//特化：查找唯一的一个
	template<typename T_iterator>
	inline typename T_iterator::value_type find_unique(const T_iterator & iterator, std::function<bool(const typename T_iterator::value_type&)> meet_one_element)
	{
		const typename T_iterator::value_type* value{};
		std::vector<typename T_iterator::value_type> result;
		for (const auto& one : iterator)
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
	//特化：查找全部
	template<typename T_iterator>
	inline std::vector<typename T_iterator::value_type> find_all(const T_iterator & iterator, std::function<bool(const typename T_iterator::value_type&)> meet_one_element)
	{
		std::vector<typename T_iterator::value_type> result;
		for (const auto& one : iterator)
		{
			if (meet_one_element(one))
				result.push_back(one);
		}
		return result;
	}
#pragma endregion
}