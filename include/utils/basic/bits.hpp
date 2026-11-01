#pragma once

#include <stdint.h>
#include <utils/type_traits.hpp>
#include <utils/assert.hpp>
#include <utils/data.hpp>
#include <utils/iterator.hpp>
#include <utils/integer_type.hpp>

namespace utils::basic::bits
{
	//获得内存块中1的个数
	//e.g.：  count_1(0x00) == 0
	//e.g.：  count_1(0xFF) == 8
	//e.g.：  count_1(0x05) == 2
	//e.g.：  count_1(0xFFFF) == 16
	template<typename T>
	inline uint64_t count_1(_In_ T val)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>, "");

		uint64_t re = 0;
		for (int i = 0; i < sizeof(T) * 8; i++)
		{
			re += val & 1;
			val >>= 1;
		}
		return re;
	}
	//特化
	inline uint64_t count_1(const std::vector<uint8_t>& data)
	{
		uint64_t re = 0;
		for (const auto b : data)
			re += count_1(b);
		return re;
	}
	
	//写入一位: 把一个整数的某一位置1或者置0
	//is_set: 是否置1
	//bit_index: 从低位开始
	template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
	void set_bit(_Inout_ T & value, _In_ const uint64_t bit_index, _In_ const bool is_set)
	{
		//简单检查
		utils_assert(bit_index < sizeof(T)*8);

		uint64_t mask = 1;
		mask <<= bit_index;
		if (is_set)
			value |= mask;
		else
			value &= ~mask;
	}
	//特化
	//bytes: 表示一个内存区
	//index: 位的索引
	//bit: 位的值
	inline void set_bit(utils::data_view_writable bytes, uint64_t index, bool bit)
	{
		//简单检查
		utils_assert(index / 8 < bytes.size());

		//要改动的字节
		uint8_t& _byte = bytes[index / 8];

		//计算mask
		uint8_t mask = 1 << (index % 8);

		//保存
		if (bit)
			_byte |= mask;
		else
			_byte &= ~mask;
	};
	//获取一位
	//bit_index: 位索引
	//return: 位的值
	template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
	bool get_bit(_In_ const T value, _In_ const uint64_t bit_index)
	{
		//简单检查
		utils_assert(bit_index < sizeof(T)*8);

		uint64_t mask = 1;
		mask <<= bit_index;
		return (value & mask) == 0 ? false : true;
	}
	//特化
	//bytes: 表示一个内存区
	//index: 位的索引
	inline bool get_bit(utils::data_view bytes, uint64_t index)
	{
		//简单检查
		utils_assert(index / 8 < bytes.size());

		//要读取的字节
		const uint8_t& _byte = bytes[index / 8];

		//计算mask
		uint8_t mask = 1 << (index % 8);

		//给出
		return (_byte & mask) != 0;
	};


	//颠倒低位指定位数的值
	//e.g.: reverse_low_bits(0b1110, 2)  ->  0b1101
	template<typename T>
	T reverse_low_bits(_In_ const T value, _In_ const uint64_t bit_n)
	{
		//简单检查
		static_assert(utils::is_integral_v<T> && std::is_unsigned_v<T>);
		static_assert(sizeof(T) <= 8);
		utils_assert(sizeof(T)*8 >= bit_n);

		T result{};
		for (uint64_t i = 0; i < bit_n; i++)
		{
			auto bit = get_bit(value, i);
			set_bit(result, bit_n-i-1, bit);
		}
		//copy其他位
		for (uint64_t i = 0; i < sizeof(T)*8 - bit_n; i++)
		{
			auto bit_index = i + bit_n;
			auto bit = get_bit(value, bit_index);
			set_bit(result, bit_index, bit);
		}
		return result;
	}

	//返回低位指定位数的值
	//e.g.: get_low_bits(0b1110, 2)  ->  0b10
	template<typename T>
	T get_low_bits(_In_ const T value, _In_ const uint64_t bit_n)
	{
		//简单检查
		static_assert(utils::is_integral_v<T> && std::is_unsigned_v<T>);
		static_assert(sizeof(T) <= 8);
		utils_assert(sizeof(T)*8 >= bit_n);

		//对于bit_n == 64的情况下,mask会生成的不对,所以直接返回原值
		if (bit_n == 64)
			return value;

		uint64_t mask = ~((~(uint64_t)0) << bit_n);
//		auto c = (uint64_t)0;
//		auto b = (~c);
//		auto a = (b << bit_n);
//		uint64_t mask = ~a;
		return (T)(((uint64_t)value) & mask);
	}
	
	//一个整数某一位是否为1
	//e.g.：   is_bit_set(0, 3) == true
	//e.g.：   is_bit_set(1, 3) == true
	//e.g.：   is_bit_set(2, 3) == false
	//e.g.：   is_bit_set(3, 3) == false
	//v2: 对调了参数顺序
	template<typename T>
	bool is_bit_set_v2(T value, uint64_t bit_index)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>, "");

		uint64_t v{};
		*(T*)&v = value;	//转为无符号的，防止右移填充1
		v >>= bit_index;
		return (v & 1) == 1;
	}

	//返回一个整数的低位连续0的个数
	//e.g.: get_low_zero_count(0b11100)  ->  2
	template<typename T>
	uint64_t get_low_zero_count(_In_ const T value)
	{
		//简单检查
		static_assert(utils::is_integral_v<T> && std::is_unsigned_v<T>);
		static_assert(sizeof(T) <= 8);

		for (const auto i : utils::iterator_index(sizeof(T)*8))
		{
			if (is_bit_set_v2(value, i))
				return i;
		}
		return sizeof(T)*8;
	}
	
	//把一些位变为0
	template<typename value_t, typename mask_t,
		typename = utils::enable_for_integer<value_t>,
		typename = utils::enable_for_integer<mask_t>>
	value_t clear(value_t value, mask_t mask)
	{
		//简单检查
		static_assert(sizeof(value_t)>= sizeof(mask_t));

		return value & ~(mask);
	}
	
	namespace inner::get_low_half
	{
		template<typename T>
		struct get_type;
		template<>
		struct get_type<u8>
		{
			using type = u4;
		};
		template<>
		struct get_type<u4>
		{
			using type = u2;
		};
		template<>
		struct get_type<u2>
		{
			using type = u1;
		};
	}
	//获得低半部分
	template<typename T, typename T_return = inner::get_low_half::get_type<T>::type>
	T_return get_low_half(T value)
	{
		return (T_return)value;
	}
	//获得高半部分
	template<typename T, typename T_return = inner::get_low_half::get_type<T>::type>
	T_return get_high_half(T value)
	{
		return (T_return)(value>>(sizeof(T_return)*8));
	}

	//获得指定连续范围内的位
	//start: 最低位
	//size: 位的个数
	//example: 要获取[15,12]之间的位，使用start=12，size=4
	template<typename T, typename = utils::enable_for_integer<T>>
	T get_bits(T value, u8 start, u8 size)
	{
		//简单检查
		static_assert(std::is_unsigned_v<T>);
		utils_assert(size > 0);
		utils_assert(start + size <= sizeof(T)*8);

		//消除右边
		value = value >> start;

		//消除左边
		value = value & ( (T)~(T)0 >> (sizeof(T)*8 - size));

		//给出
		return value;
	}

	//按有符号数的方式扩展
	//size: 目前有符号数的位的个数
	template<typename T, typename = utils::enable_for_integer<T>>
	T signed_extend(T value, u8 size)
	{
		//简单检查
		static_assert(std::is_unsigned_v<T>);
		utils_assert(size > 0);
		utils_assert(size <= sizeof(T)*8);

		//不需要更改
		if (size == sizeof(T)*8) return value;
		
		//不需要更改
		if (!get_bit(value, size-1)) return value;

		//修改左边
		value = value | ( (T)~(T)0 << size);

		//给出
		return value;
	}
	namespace set_bits::inplace
	{
		//设置指定位
		template<typename T, typename U,
			typename = utils::enable_for_integer<T>,
			typename = utils::enable_for_same<T,U>>
		inline void set(T & value, U mask)
		{
			value |= mask;
		}
		//清除指定位
		template<typename T, typename U,
			typename = utils::enable_for_integer<T>,
			typename = utils::enable_for_same<T,U>>
		inline void clear(T & value, U mask)
		{
			value &= ~mask;
		}
		//根据一个传入的bool值，来修改指定位。如果为true，则设置这些位，如果为false，则清除这些位
		template<typename T, typename U,
			typename = utils::enable_for_integer<T>,
			typename = utils::enable_for_same<T,U>>
		inline void by_bool(T & value, bool is_set, U mask)
		{
			if(is_set)
				set(value, mask);
			else
				clear(value, mask);
		}
	}
}