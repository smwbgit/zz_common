#pragma once

#include <utils/data.hpp>

namespace utils::data_encoding::uleb128
{
	inline void decode(uint64_t & value, uint64_t & used_size, std::function<uint8_t()> get_byte)
	{
		value = 0;
		used_size = 0;
		uint64_t shift = 0;
		for (uint64_t i = 0; ; i++)
		{
			uint8_t byte = get_byte();
			value |= (uint64_t)(byte & 0x7F) << shift;
			used_size++;
			
			//简单检查
			if (i == 9)
				utils_assert((byte & 0xFE) == 0);	//还剩6位未使用（10 * 7位 - 64位）

			if ((byte & 0x80) == 0)
				break;
			shift += 7;
		}
	}
	//特化
	inline void decode(uint64_t & value, uint64_t & used_size, utils::data_view data_view)
	{
		uint64_t i{};
		decode(value, used_size, [&i, &data_view]()
		{
			utils_assert(i < data_view.size());

			return data_view[i++];
		});
	}
}
namespace utils::data_encoding::sleb128
{
	inline void decode(int64_t & value, uint64_t & used_size, std::function<uint8_t()> get_byte)
	{
		value = 0;
		used_size = 0;
		uint64_t shift = 0;
		for (uint64_t i = 0; ; i++)
		{
			uint8_t byte = get_byte();
			value |= (uint64_t)(byte & 0x7F) << shift;
			used_size++;
			shift += 7;
			
			//简单检查
			if (i == 9)
				utils_assert((byte & 0xFE) == 0);	//还剩6位未使用（10 * 7位 - 64位）

			if ((byte & 0x80) == 0)
			{
				if (shift < 64 && (byte & 0x40))
					value |= (~(uint64_t)0 << shift);
				break;
			}
		}
	}
}