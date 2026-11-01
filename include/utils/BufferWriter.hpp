#pragma once

#include <fstream>
#include <string>
#include "utils/inner/writer.hpp"

namespace utils::inner::writer::buffer
{
	//为windows专门准备的快速版.如无特殊需求,考虑使用通用版本: utils::DataWriter
	class BufferWriterBase
	{
	public:
		BufferWriterBase(bool is_big_buffer = false)
		{
			//缓冲区大小
			m_buffer_max_size = is_big_buffer ? 0x10000000000 /* 1T */ : 0x100000000  /* 4G */;

			//保留内存地址
			m_buffer_addr = (uint8_t*)::VirtualAlloc(0, m_buffer_max_size, MEM_RESERVE, PAGE_READWRITE);
			utils_assert(m_buffer_addr);

			//更新状态
			m_cur_addr = m_buffer_addr;
		}
		~BufferWriterBase()
		{
			//释放内存地址
//			utils_assert(::VirtualFree(m_buffer_addr, 0, MEM_RELEASE));
			::VirtualFree(m_buffer_addr, 0, MEM_RELEASE);	//todo 看看能不能直接把地址给外边
				//todo 析构函数不让引发异常
		}
		void write(const void* data, uint64_t size)
		{
			prepare_enough_space(size);
			_write_data(data, size);
		}
		void flush()
		{
		}
		//获得当前写入位置
		void tellp(uint64_t & pos)
		{
			pos = m_cur_addr - m_buffer_addr;
		}
		//获取已写入的全部数据
		void get_buffer_data(_Out_ std::vector<uint8_t> & data)
		{
			uint64_t used_size = m_cur_addr - m_buffer_addr;
			data.assign(m_buffer_addr, m_buffer_addr + used_size);
		}
	private:
		//为了写入指定大小的数据，准备出足够的空间
		//size: 即将被写入数据的大小
		void prepare_enough_space(_In_ uint64_t size)
		{
			//当前空间够
			uint64_t need_size = (m_cur_addr - m_buffer_addr) + size;
			if (need_size <= m_buffer_size)
				return;

			//超出允许的最大缓冲大小
			if (need_size > m_buffer_max_size) //[[unlikely]]
				utils_throw();

			//新的缓冲区大小
			uint64_t new_buffer_size{};
			{
				//比需要的大一点，对齐到1M。最大不超过缓冲的最大允许大小

				new_buffer_size = need_size;

				uint64_t boundary	= 0x100000 /* 1M */;
				uint64_t mask		= ~((uint64_t)0xFFFFF);
				new_buffer_size = (new_buffer_size + boundary) & mask;

				if (new_buffer_size > m_buffer_max_size) //[[unlikely]]
					new_buffer_size = m_buffer_max_size;
			}

			//提交内存  i.e.申请内存
			m_buffer_addr = (uint8_t*)VirtualAlloc(m_buffer_addr, new_buffer_size, MEM_COMMIT, PAGE_READWRITE);
			utils_assert(m_buffer_addr);

			//更新状态
			m_buffer_size = new_buffer_size;
		}
		//在已经准备了足够的空间后，负责把数据拷贝到当前写入位置，然后更新当前写入位置
		void _write_data(const void* data, uint64_t size)
		{
			//拷贝
			memmove(m_cur_addr, data, size);

			//更新写入位置
			m_cur_addr += size;
		}
	private:
		uint8_t* m_cur_addr{};		//当前写入位置
		uint8_t* m_buffer_addr{};		//缓冲地址
		uint64_t m_buffer_size{};		//缓冲当前大小
		uint64_t m_buffer_max_size{};	//缓冲最大大小
	};


	using BufferWriter = utils::inner::writer::WithCommonFunctions<BufferWriterBase>;
}
namespace utils
{
	using BufferWriter = utils::inner::writer::buffer::BufferWriter;
}