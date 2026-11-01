#pragma once

#include <utils/basic.hpp>
#include <utils/integer_type.hpp>

namespace utils
{
	namespace inner::Index2D
	{
		class Range;
	}

	//一个二维索引
	struct Index2D
	{
	public:
		using Range = inner::Index2D::Range;
	public:
		struct s_iterator
		{
		private:
			const Index2D & m_value;
			uint64_t m_index{};
		public:
			s_iterator(const Index2D & value, uint64_t index)
				: m_value{value}
				, m_index{index}
			{
			}
			utils::Index2D operator*()
			{
				return {
					m_index % m_value.i,
					m_index / m_value.i,
				};
			}
			void operator ++()
			{
				//简单检查
				utils_assert(m_index < (uint64_t)(m_value.i * m_value.j));

				m_index++;
			}
			bool operator !=(const s_iterator & o) const
			{
				return m_index != o.m_index;
			}
		};

	public:
		int64_t i{};
		int64_t j{};
				
	public:
		Index2D()
		{
		}
		template<typename T1, typename T2>
		Index2D(T1 _i, T2 _j)
		{
			utils::basic::store_in(i, _i);
			utils::basic::store_in(j, _j);
		}
		bool operator == (const Index2D & o) const
		{
			return
				i == o.i &&
				j == o.j;
		}
		bool operator < (const Index2D & o) const
		{
			if (i != o.i)
				return i < o.i;
			else
				return j < o.j;
		}
		Index2D& operator += (const Index2D& o)
		{
			i += o.i;
			j += o.j;
			return *this;
		}
		Index2D operator + (const Index2D& o) const
		{
			Index2D re = *this;
			re += o;
			return re;
		}
		Index2D operator + (const s8 value) const
		{
			Index2D re = *this;
			re.i += value;
			re.j += value;
			return re;
		}
		Index2D& operator -= (const Index2D& o)
		{
			i -= o.i;
			j -= o.j;
			return *this;
		}
		Index2D operator - (const Index2D& o) const
		{
			Index2D re = *this;
			re -= o;
			return re;
		}
		Index2D operator - () const
		{
			return Index2D{-i,-j};
		}
		Index2D& operator /= (int64_t value)
		{
			i /= value;
			j /= value;
			return *this;
		}
		Index2D operator / (int64_t value) const
		{
			Index2D re = *this;
			re /= value;
			return re;
		}
		Index2D& operator *= (int64_t value)
		{
			i *= value;
			j *= value;
			return *this;
		}
		Index2D operator * (int64_t value) const
		{
			Index2D re = *this;
			re *= value;
			return re;
		}
		Index2D& operator *= (double value)
		{
			i = (s8)std::round(i * value);
			j = (s8)std::round(j * value);
			return *this;
		}
		Index2D operator * (double value) const
		{
			Index2D re = *this;
			re *= value;
			return re;
		}
		void shift(const Index2D& o)
		{
			*this += o;
		}
		//特化
		void shift(int64_t si, int64_t sj)
		{
			*this += Index2D{si, sj};
		}
		//返回一个shift后的值，本身不变
		Index2D return_shift(const Index2D& o) const
		{
			auto re = *this;
			re.shift(o);
			return re;
		}
		//特化
		Index2D return_shift(int64_t si, int64_t sj) const
		{
			return return_shift(Index2D{si,sj});
		}
		//把2维坐标变为1维坐标
		int64_t index_1D(Index2D size) const
		{
			return i + j * size.i;
		}
		//把一个表示2维大小的本结构变为表示同样数量的1维大小
		int64_t size_1D() const
		{
			return i * j;
		}
		//是否在指定范围/图片内
		bool is_in(Index2D size) const
		{
			//简单检查
			utils_assert(i >= 0);
			utils_assert(j >= 0);
			utils_assert(size.i >= 0);
			utils_assert(size.j >= 0);

			return i < size.i && j < size.j;
		}
		//是否全为零
		bool is_all_zero() const
		{
			return i == 0 && j == 0;
		}
		//至少有一个零
		bool is_exist_zero() const
		{
			return i == 0 || j == 0;
		}
#pragma region 迭代器
		s_iterator begin() const
		{
			return {*this, 0};
		}
		s_iterator end() const
		{
			//简单检查
			utils_assert(i >= 0);
			utils_assert(j >= 0);

			return {*this, (uint64_t)(i * j)};

		}
#pragma endregion
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & i;
			file & j;
		}
	};

	namespace inner::Index2D
	{
		//一个二维索引的范围
		class Range
		{
		public:
			struct s_iterator
			{
			private:
				const Range & m_value;
				uint64_t m_index{};
			public:
				s_iterator(const Range & value, uint64_t index)
					: m_value{value}
					, m_index{index}
				{
				}
				utils::Index2D operator*()
				{
					return {
						m_value.m_pos.i + m_index % m_value.m_size.i,
						m_value.m_pos.j + m_index / m_value.m_size.i,
					};
				}
				void operator ++()
				{
					//简单检查
					utils_assert(m_index < (uint64_t)(m_value.m_size.i * m_value.m_size.j));

					m_index++;
				}
				bool operator !=(const s_iterator & o) const
				{
					return m_index != o.m_index;
				}
			};
		private:
			utils::Index2D m_pos;
			utils::Index2D m_size;
		public:
			Range() = default;
			Range(utils::Index2D pos, utils::Index2D size)
				: m_pos{pos}
				, m_size{size}
			{
			}
#pragma region 迭代器
			s_iterator begin() const
			{
				return {*this, 0};
			}
			s_iterator end() const
			{
				//简单检查
				utils_assert(m_size.i >= 0);
				utils_assert(m_size.j >= 0);

				return {*this, (uint64_t)(m_size.i * m_size.j)};

			}
#pragma endregion
			utils::Index2D pos() const
			{
				return m_pos;
			}
			utils::Index2D size() const
			{
				return m_size;
			}
			int64_t width() const //todo 看看这个类是否需要表示size为负的情况
			{
				return m_size.i;
			}
			int64_t height() const
			{
				return m_size.j;
			}
		};
	}}
