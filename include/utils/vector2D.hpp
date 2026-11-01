#pragma once

#include <vector>
#include "utils/assert.hpp"
#include "utils/iterator.hpp"

namespace utils
{
	//提供一个二维的vector，内部用一个一维的vector保存数据
	template<typename T>
	class vector2D
	{
	public:
		//获得数据总个数
		uint64_t size() const
		{
			return m_vec.size();
		}
		//获得宽
		uint64_t width() const
		{
			return m_width;
		}
		//获得高
		uint64_t height() const
		{
			return m_height;
		}
		//获得迭代器：迭代索引 i,j
		auto iterator_index_2D() const
		{
			return utils::iterator::iterator_index_2D(m_width, m_height);
		}
		//获得迭代器：迭代一维数组的索引
		auto iterator_index_1D() const
		{
			return utils::iterator::iterator_index_1D(m_width, m_height);
		}
		//创建一个指定大小的平面
		//value：填充的值
		void assign(
			_In_ const uint64_t width,
			_In_ const uint64_t height,
			_In_ const T & value)
		{
			//填充数据
			m_vec.assign(width * height, value);

			//更新状态
			m_width = width;
			m_height = height;
		}
		//把vector里的数据当做2D数据
		//width：新的宽
		void assign(
			_In_ const std::vector<T> & values,
			_In_ const uint64_t width)
		{
			//简单检查
			utils_assert(values.size() % width == 0);

			//复制数据
			m_vec = values;

			//更新状态
			m_width = width;
			m_height = values.size() / width;
		}

		//从另一个vector2D里的指定区域里获取数据
		//values：另一个vector2D
		//start_i：起始位置
		//start_j：起始位置
		//width：区域大小
		//height：区域大小
		void assign(
			_In_ const vector2D<T> & values,
			_In_ const uint64_t start_i, _In_ const uint64_t start_j,
			_In_ const uint64_t width, _In_ const uint64_t height)
		{
			//简单检查
			utils_assert(start_i + width <= values.m_width);
			utils_assert(start_j + height <= values.m_height);

			//复制
			std::vector<T> vec;	//用新的数组，这样可以对自己使用此函数
			vec.resize(width * height);
			for (const auto i : utils::iterator_index(width))
				for (const auto j : utils::iterator_index(height))
					vec[utils::vector_function::get_index_1D(i, j, width)] = values.m_vec[values.get_index_1D(start_i + i, start_j + j)];

			//更新状态
			m_vec = std::move(vec);
			m_width = width;
			m_height = height;
		}

		//使用指定值更新指定区域的数据，不改变区域大小
		//values: 使用其中的全部值来更新
		//start_i：起始位置
		//start_j：起始位置
		//width：区域大小
		//height：区域大小
		void update(
			_In_ const std::vector<T> & values,
			_In_ const uint64_t start_i, _In_ const uint64_t start_j,
			_In_ const uint64_t width, _In_ const uint64_t height)
		{
			//简单检查
			utils_assert(start_i + width <= m_width);
			utils_assert(start_j + height <= m_height);
			
			//复制
			for (const auto i : utils::iterator_index(width))
				for (const auto j : utils::iterator_index(height))
					m_vec[get_index_1D(start_i + i, start_j + j)] = values[utils::vector_function::get_index_1D(i, j, width)];
		}
		
		//获取其中的值
		T & operator [](_In_ const utils::Index2D & index)
		{
			return m_vec[get_index_1D(index)];
		}
		//特化
		const T & operator [](_In_ const utils::Index2D & index) const
		{
			return m_vec[get_index_1D(index)];
		}
		//特化
		T & operator [](_In_ const uint64_t index)
		{
			return m_vec[index];
		}
		//特化
		const T & operator [](_In_ const uint64_t index) const
		{
			return m_vec[index];
		}

		//把形如:
		// 3 0 5 0 0 0
		// 2 2 7 0 0 0
		// 9 1 2 0 0 0
		// 0 0 0 0 0 0
		//的数据变为:
		// 3 0 5
		// 2 2 7
		// 9 1 2
		//invalid_value: 在此例中为0
		void resize_by_invalid_data(_In_ const T & invalid_value)
		{
			//遍历每行，找出每行的最长的有效长度
			uint64_t new_width{};
			for (const auto j : utils::iterator_index(m_height))
			{
				uint64_t length{};
				for (const auto i : utils::iterator_index(m_width))
				{
					if (m_vec[get_index_1D(i, j)] != invalid_value)
						length = i + 1;
				}
				new_width = (std::max)(new_width, length);
			}
			//同理遍历每列
			uint64_t new_height{};
			for (const auto i : utils::iterator_index(m_width))
			{
				uint64_t length{};
				for (const auto j : utils::iterator_index(m_height))
				{
					if (m_vec[get_index_1D(i, j)] != invalid_value)
						length = j + 1;
				}
				new_height = (std::max)(new_height, length);
			}

			//变小
			assign(*this, 0, 0, new_width, new_height);
		}


	private:
		uint64_t get_index_1D(_In_ const uint64_t i, _In_ const uint64_t j) const
		{
			return utils::vector_function::get_index_1D(i, j, m_width);
		}
		//特化
		uint64_t get_index_1D(_In_ const utils::Index2D & index) const
		{
			utils_assert(index.i >= 0);
			utils_assert(index.j >= 0);
			return get_index_1D(index.i, index.j);
		}
		std::vector<T> m_vec;
		uint64_t m_width{};
		uint64_t m_height{};
	};

}
