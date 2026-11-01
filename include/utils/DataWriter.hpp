#pragma once

#include <fstream>
#include <string>
#include "utils/inner/writer.hpp"
#include "utils/data.hpp"

namespace utils::inner::writer
{
	class DataWriterBase
	{
	private:
		utils::data m_data;	//缓冲区
		uint64_t m_pos{};	//当前写入位置

	public:
		DataWriterBase()
		{
		}
		~DataWriterBase()
		{
		}
		void write(const void* data, uint64_t size)
		{
			//进行覆盖
			if (m_pos < m_data.size())
			{
				//需要覆盖的大小
				uint64_t _size{};
				{
					if (m_pos + size <= m_data.size())
					{
						_size = size;
					}
					else
					{
						_size = m_data.size() - m_pos;
					}
				}

				//覆盖
				memmove(m_data.addr() + m_pos, data, _size);

				//更新状态
				data = (uint8_t*)data + _size;
				size -= _size;
				m_pos += _size;
			}

			//剩下的进行追加
			m_data.append({data, size});

			//更新状态
			m_pos += size;
		}
		void flush()
		{
		}
		//获得当前写入位置
		void tellp(_Out_ uint64_t & pos)
		{
			pos = m_pos;
		}
		//设置当前写入位置
		//note: 如果写入位置不在数据区的末尾, 则write()会覆盖已有数据
		void seekp(_In_ uint64_t pos)
		{
			//简单检查
			utils_assert(pos <= m_data.size());

			m_pos = pos;
		}
		//获取写入的数据
		//is_clear: 是否清空
		utils::data get_data(_In_ bool is_clear = true)
		{
			utils::data data;
			if(is_clear)
			{
				data = std::move(m_data);
				m_data.clear();
				m_pos = 0;
			}
			else
			{
				data = m_data;
			}
			return data;
		}
		//特化
		void get_data(_Out_ utils::data & data)
		{
			data = m_data;
		}
		//特化
		void get_data_and_clear(_Out_ utils::data & data)
		{
			data = std::move(m_data);
			m_data.clear();
			m_pos = 0;
		}
		//获得写入的数据
		void get_text(_Out_ std::string & text)
		{
			text.assign((char*)m_data.addr(), m_data.size());
		}
		//清空
		void clear()
		{
			m_data.clear();
			m_pos = 0;
		}
		//写入一些字符，直到对齐
		//note: 从当前写入位置算
		//note: \n和\r\n都能识别
		//start: 起始对齐位置
		//step: 每次往后挪动的距离
		//min_distance: 前面的文字离对齐位置的最小距离
		//c: 填充字符
		void write_padding_text(_In_ uint64_t start, _In_ uint64_t step, _In_ uint64_t min_distance, _In_ char c)
		{
			//准备函数
			auto get_cur_column_pos = [&]() -> uint64_t
			{
				//从后往前寻找，最近的一个\n
				for (uint64_t i = m_pos - 1; i >= 0; i--)
				{
					auto cc = m_data[i];
					if (m_data[i] == '\n')
						return m_pos - i - 1;
				}
				return m_pos;
			};

			//获得此行已经写了多少字符
			uint64_t n = get_cur_column_pos();

			//计算对齐位置
			uint64_t pos{};
			if (n + min_distance <= start)
			{
				pos = start;
			}
			else
			{
				auto times = (n + min_distance - start + step - 1) / step;
				pos = start + step * times;
			}

			//计算还要写多少个字符
			uint64_t _n = pos - n;

			//准备数据
			std::vector<char> text;
			text.resize(_n, c);

			//写入
			write(&text[0], text.size());
		}
	};


	using DataWriter = utils::inner::writer::WithCommonFunctions<DataWriterBase>;
}
namespace utils
{
	using DataWriter = utils::inner::writer::DataWriter;
}