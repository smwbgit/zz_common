#pragma once

#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/container_hash/hash.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>

namespace utils::hash
{
	template<typename T>
	std::size_t hash_value(const T & value)
	{
		return std::hash<T>()(value);
	}

	//todo 此处通不过编译
	#if 0
		template <class T>
		using hash_combine = boost::hash_combine;
	#endif
}

#if 0
	template<typename T>
	inline void hash_combine(std::size_t& seed, const T& val)
	{
		seed ^= std::hash<T>()(val)+0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
#endif

#if 0
namespace utils::hash_function
{
	namespace inner
	{
		template<typename T>
		inline void hash_val(std::size_t& seed, T&& val)
		{
			boost::hash_combine(seed, std::forward<T>(val));
		}

		template<typename T, typename... Args>
		inline void hash_val(std::size_t& seed, T&& val, Args&&... args)
		{
			boost::hash_combine(seed, std::forward<T>(val));
			utils::hash_function::inner::hash_val(seed, std::forward<Args>(args)...);
		}
	}

	//对一系列值进行求hash，然后返回结果
	template<typename... Args>
	inline std::size_t hash_val(Args && ...args)
	{
		std::size_t seed = 0;
		inner::hash_val(seed, std::forward<Args>(args)...);
		return seed;
	}
}
#endif