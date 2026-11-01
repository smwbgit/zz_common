#pragma once

#include <type_traits>
#include <vector>
#include <utils/assert.hpp>

namespace utils::basic
{
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, void>::type change_byte_order(T & value)	//对普通类型的转换
	{
		static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "");
		switch (sizeof(T))
		{
		case 1: return;
		case 2:
			std::swap(*(((char*)&value) + 0), *(((char*)&value) + 1));
			return;
		case 4:
			std::swap(*(((char*)&value) + 0), *(((char*)&value) + 3));
			std::swap(*(((char*)&value) + 1), *(((char*)&value) + 2));
			return;
		case 8:
			std::swap(*(((char*)&value) + 0), *(((char*)&value) + 7));
			std::swap(*(((char*)&value) + 1), *(((char*)&value) + 6));
			std::swap(*(((char*)&value) + 2), *(((char*)&value) + 5));
			std::swap(*(((char*)&value) + 3), *(((char*)&value) + 4));
			return;
		}
		utils_throw();
	}
	template<typename T>
	typename std::enable_if<std::is_class<T>::value, void>::type change_byte_order(T & value)	//对自定义类型的转换
	{
		value.change_byte_order();
	}
	template<typename T>
	void change_byte_order(T* values, uint64_t value_n)	//把一块内存数据当成T类型的数据来转换
	{
		for (uint64_t i = 0; i < value_n; i++)
		{
			auto & value = values[i];
			change_byte_order(value);
		}
	}
	//针对void* 指针的特化
	template<typename T, typename Pointer_T>
	void change_byte_order(Pointer_T* data, uint64_t size_in_bytes)	//把一块内存数据当成T类型的数据来转换
	{
		static_assert(
			std::is_same_v<Pointer_T, void> ||
			std::is_same_v<Pointer_T, uint8_t>
			);

		utils_assert(size_in_bytes % sizeof(T) == 0);

		change_byte_order<T>((T*)data, size_in_bytes / sizeof(T));
	}
	template<typename T, std::size_t N>
	void change_byte_order(T (&values)[N])	//对数组的转换
	{
		change_byte_order((T*)values, N);
	}
	template<typename T>
	void change_byte_order(std::vector<T> & values)	//对vector的转换
	{
		if (values.empty())
			return;
		change_byte_order(values.data(), values.size());
	}
	template<>
	inline void change_byte_order<uint8_t>(std::vector<uint8_t> & values)	//加快运行速度
	{
		(void)values;
	}
	template<typename T>
	void change_byte_order(std::vector<uint8_t> & values)	//把一块内存数据当成T类型的数据来转换
	{
		if (values.empty()) return;

		if (values.size() % sizeof(T) != 0)
			utils_throw();

		change_byte_order((T*)values.data(), values.size() / sizeof(T));
	}
}
