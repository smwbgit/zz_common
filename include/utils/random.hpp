#pragma once

#include <random>
#include <utils/data.hpp>
#include <utils/vector.hpp>
#include <utils/on_leave_scope.hpp>

namespace utils::random
{
	//生成随机数
	//note: ai生成后整理
	//size: 指定需要的字节数
	inline utils::data make_random(_In_ uint64_t size)
	{
		if (size == 0)
			return {};

		// 播种：random_device 提供熵源，seed_seq 填满 mt19937 的内部状态
		std::random_device rd;
		std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
		std::mt19937 gen(seed);

		utils::data data;
		data.resize(size);

		uint8_t* ptr = data.addr();
		uint64_t i = 0;

		// 每次生成一个 32 位随机数，拆成 4 字节写入
		for (; i + 4 <= size; i += 4)
		{
			uint32_t r = gen();
			ptr[i]     = static_cast<uint8_t>(r);
			ptr[i + 1] = static_cast<uint8_t>(r >> 8);
			ptr[i + 2] = static_cast<uint8_t>(r >> 16);
			ptr[i + 3] = static_cast<uint8_t>(r >> 24);
		}

		// 处理末尾不足 4 字节的部分
		if (i < size)
		{
			uint32_t r = gen();
			for (uint64_t shift = 0; i < size; ++i, shift += 8)
			{
				ptr[i] = static_cast<uint8_t>(r >> shift);
			}
		}

		return data;
	}
	//洗牌
	inline void get_shuffle_index(_Out_ std::vector<uint64_t> & index, _In_ uint64_t size)
	{
		//refer: https://www.cnblogs.com/AlbertP/p/10727843.html
		
		index.resize(size);
		for(const auto i : utils::iterator_index(size))
			index[i]=i;
			
		for(const auto i : utils::iterator_index(size))
		{
			uint64_t new_index = rand()%(i+1);	//todo 看看怎么把rand()换成更为随机的
			std::swap(index[i], index[new_index]);
		}
	}
	//随机排序
	template<typename T>
	void random_sort(std::vector<T> & vec)
	{
		std::vector<uint64_t> index;
		utils::random::get_shuffle_index(index, vec.size());

		std::vector<T> _new;
		_new.resize(vec.size());
		for(const auto i : utils::iterator_index(vec.size()))
		{
			_new[i]= std::move(vec[index[i]]);
		}
		vec = std::move(_new);
	}
}