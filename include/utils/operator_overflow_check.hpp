#pragma once

#include <chrono>
#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/container_hash/hash.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/set.hpp>
#include <utils/basic.hpp>
#include <utils/assert.hpp>

//运算溢出检查
//return: 溢出返回false
namespace utils::operator_overflow_check
{
	//加法
	template<typename result_T, typename operator_0_T, typename operator_1_T>
	inline bool add(_In_ result_T result, _In_ operator_0_T operator_0, _In_ operator_1_T operator_1);
	//特化
	template<>
	inline bool add<uint64_t, uint64_t, int64_t>(_In_ uint64_t result, _In_ uint64_t operator_0, _In_ int64_t operator_1)
	{
		if (operator_1 >= 0)
			return operator_0 + (uint64_t)operator_1 >= operator_0;
		else
			return operator_0 >= (uint64_t)(-operator_1);
	}
	//特化
	template<>
	inline bool add<uint64_t, uint64_t, uint64_t>(_In_ uint64_t result, _In_ uint64_t operator_0, _In_ uint64_t operator_1)
	{
		return operator_0 + operator_1 >= operator_0;
	}
}
