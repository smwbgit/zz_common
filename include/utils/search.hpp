#pragma once

#include <vector>

namespace utils::search
{
	//在一个内存块中搜索特定数据
	//data: 整个内存块
	//sub_data: 要搜索的数据
	inline bool is_exist(_In_ const std::vector<uint8_t> & sub_data, _In_ const std::vector<uint8_t> & data)
	{
		for (std::size_t i = sub_data.size() - 1; i < data.size(); i++)
		{
			if (memcmp(&data[i], sub_data.data(), sub_data.size()) == 0)
				return true;
		}
		return false;
	}

	//在内存块中查找特定数据，返回第一次出现的位置
	//返回值：是否找到
	//pos：是否找到
	//data: 整个内存块
	//sub_data: 要搜索的数据
	inline bool find(_Out_ uint64_t & pos, _In_ const std::vector<uint8_t> & sub_data, _In_ const std::vector<uint8_t> & data)
	{
		for (std::size_t i = sub_data.size() - 1; i < data.size(); i++)
		{
			if (memcmp(&data[i - (sub_data.size() - 1)], sub_data.data(), sub_data.size()) == 0)
			{
				pos = i - (sub_data.size() - 1);
				return true;
			}
		}
		return false;
	}

	//在内存块中查找特定数据，返回所有出现的位置
	//positions：所有出现的位置
	//data_addr, data_size: 整个内存块
	//sub_data: 要搜索的数据
	inline void find_all(_Out_ std::vector<uint64_t> & positions, _In_ const std::vector<uint8_t> & sub_data, _In_ const uint8_t* data_addr, _In_ const uint64_t data_size)
	{
		positions.clear();

		for (const auto i : utils::iterator_index(data_size))
		{
			if (i + sub_data.size() > data_size)
				break;

			if (memcmp(data_addr + i, sub_data.data(), sub_data.size()) == 0)
			{
				//保存
				positions.push_back(i);
			}
		}
	}
	//特化
	inline void find_all(_Out_ std::vector<uint64_t> & positions, _In_ const std::vector<uint8_t> & sub_data, _In_ const std::vector<uint8_t> & data)
	{
		return find_all(positions, sub_data, data.empty() ? nullptr : data.data(), data.size());
	}
}
