#pragma once

#include <stdint.h>
#include <vector>
#include <utils/assert.hpp>
#include <utils/data.hpp>
#include <utils/type_traits.hpp>
#include <utils/Range1D.hpp>
#include <utils/inner/reader.hpp>

namespace utils::inner::DataReader
{
	class DataReaderBase
	{
	private:
		const uint8_t* m_data_addr{};
		const uint64_t m_data_size{};
		uint64_t m_index{};		//当前读取位置
	public:
		DataReaderBase(_In_ utils::data_view data)
			: m_data_addr{data.empty() ? nullptr : data.addr()}
			, m_data_size{data.size()}
		{
		}
		void read(void* data, uint64_t size)
		{
			//读取的长度为零
			if (size == 0)
			{
				//无动作
				return;
			}

			//简单检查
			utils_assert(m_index + size <= m_data_size);

			//给出
			memmove(data, m_data_addr + m_index, size);

			//更新状态
			m_index += size;
		}
		//获得当前位置
		void tellg(uint64_t & pos)
		{
			//给出
			pos = m_index;
		}
		//设置当前位置
		void seekg(uint64_t pos)
		{
			//简单检查
			utils_assert(pos <= m_data_size);

			//保存
			m_index = pos;
		}
		//获得block的总大小
		void get_pool_size(uint64_t & pool_size)
		{
			//给出
			pool_size = m_data_size;
		}
	};

	using DataReader = utils::inner::reader::WithCommonFunctions<DataReaderBase>;
}

namespace utils
{
	//note: 以类似读取文件的方式，读取给定数据
	using DataReader = utils::inner::DataReader::DataReader;
}

#if 0//todo 看看这个版本，为什么舍弃了 utils::inner::reader::WithCommonFunctions
namespace utils
{
	//提供常用的读取函数
	class DataReader
	{
		//请指定
	public:
		std::function<void(uint64_t position, uint8_t* buffer, uint64_t size)> m_read_random;
		std::function<uint64_t()> m_get_readable_size;

	private:
		uint64_t m_read_position{};
		
		//最基本函数，是其他函数的基础
	public:
		void read(uint8_t* buffer, uint64_t size)
		{
			m_read_random(m_read_position, buffer, size);
			m_read_position += size;
		}
		uint64_t get_read_position()
		{
			return m_read_position;
		}
		void set_read_position(uint64_t position)
		{
			m_read_position = position;
		}
		uint64_t get_readable_size()
		{
			return m_get_readable_size();
		}

	public:
		DataReader()
		{
		}
		//简单包装
		DataReader(utils::data & data)
		{
			m_read_random = [&data](uint64_t position, uint8_t* buffer, uint64_t size)
			{
				//简单检查
				utils_assert(utils::Range1D(0, data.size()).is_include(position));
				utils_assert(utils::Range1D(0, data.size()).is_include(position + size - 1));

				memmove(buffer, data.addr() + position, size);
			};
			m_get_readable_size = [&data, this]() -> uint64_t
			{
				//简单检查
				utils_assert(m_read_position <= data.size());

				return data.size() - m_read_position;
			};
		}

	public:
		//读取一个基本类型或者结构体
		template<typename T>
			requires utils::is_arithmetic_v<T> || std::is_trivially_copyable_v<T>
		void read(T & value)
		{
			read((uint8_t*)&value, sizeof(T));
		}
		//读取: 存入std::vector<T>
		template<typename T>
		void read(_Out_ std::vector<T> & data, _In_ const uint64_t element_n)
		{
			data.resize(element_n);
			if (data.empty())
				return;
			read((uint8_t*)data.data(), data.size() * sizeof(T));
		}
		//读取: 存入utils::data
		void read(_Out_ utils::data & data, _In_ const uint64_t size)
		{
			data.resize(size);
			if (data.empty())
				return;
			read(data.addr(), data.size());
		}
		//读取所有数据
		utils::data read_all()
		{
			uint64_t size = get_readable_size() + get_read_position();
			set_read_position(0);
			utils::data data;
			read(data, size);
			return data;
		}
		//读取可能处于边界的数据
		void read_boundary(_Out_ utils::data & data, _In_ const uint64_t size)
		{
			auto _size = size;
			while(1)
			{
				if(!_size)
				{
					data.clear(); return;
				}

				if (utils_try(read(data, _size)))
					return;

				_size --;
			}
		}
	};
}
#endif