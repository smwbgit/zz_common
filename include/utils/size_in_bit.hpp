#pragma once

#include <stdint.h>

namespace utils
{
	//位的个数
	class size_in_bit
	{
	private:
		uint64_t m_size{};

	public:
		size_in_bit()
		{
		}
		//note: 不提供隐式转换
		explicit size_in_bit(uint64_t size)
			: m_size{size}
		{
		}

	public:
		auto operator<=>(const size_in_bit &) const = default;

	public:
		uint64_t value() const
		{
			return m_size;
		}
	};
	
	//字节的个数
	class size_in_byte
	{
	private:
		uint64_t m_size{};

	public:
		size_in_byte()
		{
		}
		//note: 不提供隐式转换
		explicit size_in_byte(uint64_t size)
			: m_size{size}
		{
		}

	public:
		auto operator<=>(const size_in_byte &) const = default;

	public:
		uint64_t value() const
		{
			return m_size;
		}
	};
}