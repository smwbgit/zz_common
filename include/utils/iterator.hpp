#pragma once

#include <utility>
#include <optional>
#include "utils/type_traits.hpp"
#include "utils/Index2D.hpp"
#include "utils/basic.hpp"

//公共模板
namespace utils::iterator::inner
{
	template<typename CustomIterator_T, typename Container_T>
	class Container
	{
	public:
		Container(Container_T & container)
			: m_container{container}
		{
		}
		auto begin()
		{
			return CustomIterator_T{m_container};
		}
		auto end()
		{
			return CustomIterator_T{m_container, true};
		}
	private:
		Container_T & m_container;
	};

	//迭代器的通用模板
	template<typename CustomIterator_T, typename Container_T>
	auto basic_iterator(const Container_T & container)
	{
		//简单检查
		static_assert(std::is_same_v<std::remove_volatile_t<Container_T>, Container_T>);

		//去掉const后传递container
		return inner::Container<CustomIterator_T, Container_T>{const_cast<Container_T &>(container)};
	}
}

//iterator_index		//todo: 使用公共模板重写
namespace utils::iterator
{
	namespace inner::iterator_index
	{
		class iterator
		{
		public:
			iterator(uint64_t i)
				: m_i {i}
			{
			}
			uint64_t operator*()
			{
				return m_i;
			}
			void operator++()
			{
				m_i ++;
			}
			bool operator!=(const iterator & o) const
			{
				return m_i != o.m_i;
			}
		private:
			uint64_t m_i{};
		};

		class iterators
		{
		private:
			uint64_t m_n{};

		public:
			iterators(uint64_t n)
				: m_n{n}
			{
			}
				
			iterator begin() const
			{
				return{0};
			}
			iterator end() const
			{
				return{m_n};
			}
		};
	}

	//遍历指定容器的各个索引
	//举例：
	//	std::vector<int> a;
	//	a.resize(10);
	//	for (auto i : utils::iterator_index(a))
	//	{
	//		// i 从0到9
	//	}
	template<typename Container_T, typename std::enable_if<utils::is_integral<Container_T>::value == false, int>::type unused = 0>
	inner::iterator_index::iterators iterator_index(const Container_T & container)
	{
		return {container.size()};
	}
	//特化:
	//遍历指定大小的各个索引
	//举例：
	//	for (auto i : utils::iterator_index(10))
	//	{
	//		// i 从0到9
	//	}
	template<typename T, typename std::enable_if<utils::is_integral<T>::value, int>::type unused = 0>
	inner::iterator_index::iterators iterator_index(const T & size)
	{
		uint64_t _size{};
		utils::basic::store_in(_size, size);
		return {_size};
	}
}

//iterator_and_give_me_i
namespace utils::iterator
{
	namespace inner::iterator_and_give_me_i
	{
		template<typename Container_T>
		class Entry
		{
		public:
			using Value_T = typename Container_T::value_type;

		public:
			Entry(Value_T & value, std::size_t i)
				: value{value}
				, i{i}
			{
			}

			Value_T & value;
			std::size_t i;
		};

		template<typename Container_T>
		class Iterator
		{
		public:
			Iterator(Container_T & container, bool is_end = false)
				: m_container{container}
			{
				if (is_end)
				{
					m_i = container.size();
				}
				else
				{
					m_i = 0;
				}
			}
			void operator ++()
			{
				m_i++;
			}
			bool operator !=(const Iterator & o) const
			{
				return m_i != o.m_i;
			}
			Entry<Container_T> operator* ()	//todo: 这里看看能不能改为返回引用, 使得for里的auto后边可以使用&
			{
				return Entry<Container_T>{*(m_container.begin() + m_i), m_i};
			}
		private:
			Container_T & m_container;
			std::size_t m_i{};
		};
	}

	//循环一个容器，每循环一次，给出 { value_t & value, std::size_t i} 这样一个结构体
	//用法示例:
	//	for (/*const*/ auto e : utils::iterator_and_give_me_i(std::vector<float>{10,20,30}))
	//	{
	//		int i = e.i;	//0, 1, 2
	//		float v = e.value;	//10, 20, 30
	// 
	//		e.value = 11;	//can change the value
	//	}
	template<typename Container_T>
	auto iterator_and_give_me_i(const Container_T & container)
	{
		return inner::basic_iterator<inner::iterator_and_give_me_i::Iterator<Container_T>>(container);
	}
}
//iterator_combination
namespace utils::iterator
{
	namespace inner::iterator_combination
	{
		template<typename Container_T>
		class Entry
		{
		public:
			using Value_T = typename Container_T::value_type;

		public:
			struct s_entry
			{
				std::size_t i{};
				Value_T & value;
			};

		public:
			s_entry first;
			s_entry second;

		public:
			Entry(s_entry first, s_entry second)
				: first{first}
				, second{second}
			{
			}
		};

		struct s_index
		{
			std::size_t i{};
			std::size_t j{};
		};

		template<typename Container_T>
		class Iterator
		{
		public:
			Iterator(Container_T & container, bool is_end = false)
				: m_container{container}
			{
				if (container.size() < 2)
				{
					m_index.i = 0;
					m_index.j = 0;
				}
				else if (is_end)
				{
					m_index.i = container.size() - 1;
					m_index.j = container.size();
				}
				else
				{
					m_index.i = 0;
					m_index.j = 1;
				}
			}
			void operator ++()
			{
				//简单检查
				utils_assert(m_container.size() >= 2);

				m_index.j ++;
				if (m_index.j >= m_container.size())
				{
					m_index.i++;
					m_index.j = m_index.i + 1;
				}
			}
			bool operator !=(const Iterator & o) const
			{
				return m_index.i != o.m_index.i || m_index.j != o.m_index.j;
			}
			Entry<Container_T> operator* ()
			{
				auto get_iter = [&](uint64_t index)
				{
					auto iter = m_container.begin();
					for (auto i : utils::iterator::iterator_index(index))
					{
						iter++;
					}
					return iter;
				};
				auto iter_i = get_iter(m_index.i);
				auto iter_j = get_iter(m_index.j);
				return Entry<Container_T>{{m_index.i, *iter_i}, {m_index.j, *iter_j}};
			}
		private:
			Container_T & m_container;
			s_index m_index{};
		};
	}

	//循环一个容器的节点对     e.g.   vec = {0,1,2,3}  则给出 {0,1}，{0,2}，{0,3}，{1,2}，{1,3}，{2,3}
	//用法示例:
	//	for (/*const*/ auto c : utils::iterator_combination(std::vector<float>{10,20,30}))
	//	{
	//		float v0 = c.first.value;		//10, 10, 20
	//		float v1 = c.second.value;		//20, 30, 30
	//		std::size_t i0 = c.first.i;		//0,  0,  1
	//		std::size_t i1 = c.second.i;	//1,  2,  2
	//	}
	template<typename Container_T>
	auto iterator_combination(Container_T & container)
	{
		return inner::basic_iterator<inner::iterator_combination::Iterator<Container_T>>(container);
	}
}

//s_iterator_index<>	//todo: 使用公共模板重写
namespace utils::iterator
{
	//用编译期常量依次调用s_fn_T::call()，一共调用N次
	//相当于:
	//		s_fn_T::call<0>(1,2,3);
	//		s_fn_T::call<1>(1,2,3);
	//		...
	//		s_fn_T::call<N-1>(1,2,3);
	//
	//用法举例:
	//		struct s_fn_init
	//		{
	//			template<std::size_t index>
	//			static void call(int i, int j, int k)
	//			{
	//				printf("%d %d %d %d\n", index, i, j, k);
	//			};
	//		};
	//		utils::iterator::s_iterator_index<10, s_fn_init>::iterator(1,2,3);
	//
	template<std::size_t N, typename s_fn_T>
	struct s_iterator_index
	{
	private:
		template<typename ... _Args>
		struct s_iterator
		{
		private:
			template<std::size_t index, typename unused_T>	//note: 嵌套类中不能完全特化,但可以部分特化, 所以加上了个unused_T
			struct s_func
			{
				static void _do(_In_ _Args && ... args)
				{
					//调用指定函数
					s_fn_T::template call<index>(std::forward<_Args>(args)...);

					//下一个
					return iterator<index+1>(std::forward<_Args>(args)...);
				}
			};
			template<typename unused_T>
			struct s_func<N, unused_T>
			{
				static void _do(_In_ _Args && ... args)
				{
					//已经循环完所有的
				}
			};

		public:
			template<std::size_t index>
			static void iterator(_In_ _Args && ... args)
			{
				//转发到具体实现
				return s_func<index,void>::_do(std::forward<_Args>(args)...);
			}
		};
	public:
		template<typename ... _Args>
		static void iterator(_In_ _Args && ... args)
		{
			s_iterator<_Args...>::template iterator<0>(std::forward<_Args>(args)...);	//todo 把args存到成员变量tuple里
		}
	};
}

//iterator_neighbour	//todo: 使用公共模板重写
namespace utils::iterator
{
	namespace inner::iterator_neighbour
	{
		template<typename Value_T>
		class Entry
		{
		public:
			Entry(Value_T & first, Value_T & second)
				: first{first}
				, second{second}
			{
			}

			Value_T & first;
			Value_T & second;
		};

		template<typename Iterator_T, typename Value_T>
		class Iterator
		{
		public:
			Iterator(Iterator_T iter, std::size_t i)
				: m_iter{iter}
				, m_i{i}
			{
			}
			void operator ++()
			{
				m_iter++;
				m_i++;
			}
			bool operator !=(const Iterator & o) const
			{
				return m_i != o.m_i;
			}
			Entry<Value_T> operator* ()
			{
				Iterator_T next = m_iter;
				next++;
				return Entry<Value_T>{*m_iter, *next};
			}
		private:
			Iterator_T m_iter;
			std::size_t m_i;
		};
		template<typename Container_T, typename Iterator_T, typename Value_T>
		class Container
		{
		public:
			Container(Container_T & container)
				: m_container{container}
			{
			}
			Iterator<Iterator_T, Value_T> begin() const
			{
				return Iterator<Iterator_T, Value_T>{m_container.begin(), 0};
			}
			Iterator<Iterator_T, Value_T> end() const
			{
				return Iterator<Iterator_T, Value_T>{m_container.end(), (m_container.empty() ? 0 : m_container.size() - 1)};
			}
		private:
			Container_T & m_container;
		};
	}

	//循环一个容器的每组邻居节点     e.g.   vec = {0,1,2,3}  则给出 {0,1}，{1,2}，{2,3}
	//用法示例:
	//	for (auto & n : utils::iterator_neighbour(std::vector<float>{10,20,30}))
	//	{
	//		float v0 = n.first;		//10         下一组：// 20
	//		float v1 = n.second;	//20                 // 30
	//	}
	template<typename Container_T>
	auto iterator_neighbour(Container_T & container) -> inner::iterator_neighbour::Container<Container_T, typename Container_T::iterator, typename Container_T::value_type>
	{
		return inner::iterator_neighbour::Container<Container_T, typename Container_T::iterator, typename Container_T::value_type>{container};
	}
	//const版本
	template<typename Container_T>
	auto iterator_neighbour(const Container_T & container) -> inner::iterator_neighbour::Container<typename std::add_const<Container_T>::type, typename Container_T::const_iterator, typename std::add_const<typename Container_T::value_type>::type>
	{
		return inner::iterator_neighbour::Container<typename std::add_const<Container_T>::type, typename Container_T::const_iterator, typename std::add_const<typename Container_T::value_type>::type>{container};
	}
}


//index_2D::iterator
namespace utils::iterator::index_2D
{
	namespace inner::iterator
	{
		class iterator
		{
		public:
			iterator(uint64_t w, uint64_t i)
				: m_w{w}
				, m_i{i}
			{
			}
			utils::Index2D operator*()
			{
				return {m_i % m_w, m_i / m_w};	//todo 这里看看不用除法，多加2个成员变量，直接给出
			}
			void operator++()
			{
				m_i ++;
			}
			bool operator!=(const iterator & o) const
			{
				return m_i != o.m_i;
			}
		private:
			uint64_t m_w{};
			uint64_t m_i{};
		};

		class iterators
		{
		public:
			using value_type = utils::Index2D;

		private:
			uint64_t m_w{};
			uint64_t m_h{};

		public:
			iterators(uint64_t w, uint64_t h)
				: m_w{w}
				, m_h{h}
			{
				//简单检查
				utils_assert(utils::basic::can::mul(w, h));
				utils_assert(utils::basic::can::store_in<int64_t>(w));
				utils_assert(utils::basic::can::store_in<int64_t>(h));
			}
				
			iterator begin() const
			{
				return{m_w, 0};
			}
			iterator end() const
			{
				return{m_w, m_w * m_h};
			}
		};
	}

	//遍历指定大小的平面的各个索引
	//举例：
	//	for (auto index : utils::iterator_index_2D(10, 20))
	//	{
	//		// index.i 从0到9
	//		// index.j 从0到19
	//		// 总共循环200次
	//	}
	template<typename T, typename std::enable_if<utils::is_integral<T>::value, int>::type unused = 0>
	inner::iterator::iterators iterator(const T & width, const T & height)
	{
		uint64_t _width{};
		uint64_t _height{};
		utils::basic::store_in(_width, width);
		utils::basic::store_in(_height, height);
		return {_width, _height};
	}
}

//iterator_center
namespace utils::iterator::index_2D
{
	namespace inner::iterator_center
	{
		class iterator
		{
		private:
			int64_t m_center_x{};
			int64_t m_center_y{};
			uint64_t m_r{};
			uint64_t m_cur_r{};
			uint64_t m_cur_edge{};
			uint64_t m_cur_i{};

		public:
			iterator(int64_t center_x, int64_t center_y, uint64_t r,
				uint64_t cur_r, uint64_t cur_edge, uint64_t cur_i)
				: m_center_x{center_x}
				, m_center_y{center_y}
				, m_r{r}
				, m_cur_r{cur_r}
				, m_cur_edge{cur_edge}
				, m_cur_i{cur_i}
			{
			}
			utils::Index2D operator*()
			{
				if(m_cur_r == 0)
				{
					return {m_center_x, m_center_y};
				}
				else
				{
					int64_t shift_x{};
					int64_t shift_y{};
					switch(m_cur_edge)
					{
					case 0: shift_x=m_cur_i;break;
					case 1: shift_y=m_cur_i;break;
					case 2: shift_x=m_cur_i;break;
					case 3: shift_y=m_cur_i;break;
					default:	utils_throw();
					}
					int64_t begin_x{};
					int64_t begin_y{};
					switch(m_cur_edge)
					{
					case 0: begin_x = m_center_x-m_cur_r; begin_y = m_center_y-m_cur_r;break;
					case 1: begin_x = m_center_x+m_cur_r; begin_y = m_center_y-m_cur_r;break;
					case 2: begin_x = m_center_x-m_cur_r+1; begin_y = m_center_y+m_cur_r;break;
					case 3: begin_x = m_center_x-m_cur_r; begin_y = m_center_y-m_cur_r+1;break;
					default:	utils_throw();
					}

					return {begin_x + shift_x, begin_y + shift_y};
				}
			}
			void operator++()
			{
				//简单检查
				utils_assert(m_cur_r < m_r);

				if(m_cur_r == 0)
				{
					m_cur_r = 1; m_cur_edge = 0; m_cur_i = 0;
				}
				else
				{
					m_cur_i++;
					if(m_cur_i >= 2*m_cur_r)
					{
						m_cur_edge++; m_cur_i=0;
						if(m_cur_edge >= 4)
						{
							m_cur_r++;m_cur_edge=0;
						}
					}
				}
			}
			bool operator!=(const iterator & o) const
			{
				return 
					m_cur_r != o.m_cur_r ||
					m_cur_edge != o.m_cur_edge ||
					m_cur_i != o.m_cur_i;
			}
		};

		class iterators
		{
		public:
			using value_type = utils::Index2D;

		private:
			int64_t m_center_x{};
			int64_t m_center_y{};
			uint64_t m_r{};

		public:
			iterators(int64_t center_x, int64_t center_y, uint64_t r)
				: m_center_x{center_x}
				, m_center_y{center_y}
				, m_r{r}
			{
			}
				
			iterator begin() const
			{
				return{m_center_x, m_center_y, m_r, 0, 0, 0};
			}
			iterator end() const
			{
				return{m_center_x, m_center_y, m_r, m_r, 0, 0};
			}
		};
	}

	//以某一点为中心向四周扩散
	//举例：
	//	for (auto index : utils::iterator::index_2D::iterator_center(10, 20, 5))
	//	{
	//		// index.i 为10,9,11,8,12,...,6,14
	//		// index.j 为20,19,21,18,22,...,16,24
	//		// 总共循环81次
	//	}
	template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
	inner::iterator_center::iterators iterator_center(const T & center_x, const T & center_y, const T & r)
	{
		int64_t _center_x{};
		int64_t _center_y{};
		uint64_t _r{};
		utils::basic::store_in(_center_x, center_x);
		utils::basic::store_in(_center_y, center_y);
		utils::basic::store_in(_r, r);
		return {_center_x, _center_y, _r};
	}
}

//内存中的数组：需要指定它的类型, 起始位置和大小
namespace utils::iterator
{
	namespace inner::array
	{
		template<typename T>
		class range
		{
		private:
			T* m_array{};
			uint64_t m_size{};

		public:
			range(T* array, uint64_t size)
				: m_array{array}
				, m_size{size}
			{
				//简单检查
				utils_assert(array || size == 0);
			}
			T* begin() const
			{
				return m_array;
			}
			T* end() const
			{
				return m_array + m_size;
			}
			uint64_t size() const
			{
				return m_size;
			}
		};
	}

	//遍历数组
	template<typename T>
	inner::array::range<T> array(T* array, uint64_t size)
	{
		return {array, size};
	}
}

//iterator_index_1D		//todo: 使用公共模板重写
namespace utils::iterator
{
	namespace inner::iterator_index_1D
	{
		class iterator
		{
		public:
			iterator(uint64_t i)
				: m_i{i}
			{
			}
			uint64_t operator*()
			{
				return m_i;
			}
			void operator++()
			{
				m_i ++;
			}
			bool operator!=(const iterator & o) const
			{
				return m_i != o.m_i;
			}
		private:
			uint64_t m_i{};
		};

		class iterators
		{
		private:
			uint64_t m_w{};
			uint64_t m_h{};

		public:
			iterators(uint64_t w, uint64_t h)
				: m_w{w}
				, m_h{h}
			{
				//简单检查
				utils_assert(utils::basic::can::mul(w, h));
			}
				
			iterator begin() const
			{
				return{0};
			}
			iterator end() const
			{
				return{m_w * m_h};
			}
		};
	}

	//遍历指定大小的平面的一维索引
	//举例：
	//	for (auto i : utils::iterator_index_1D(10, 20))
	//	{
	//		// i 从0到199
	//	}
	template<typename T, typename std::enable_if<utils::is_integral<T>::value, int>::type unused = 0>
	inner::iterator_index_1D::iterators iterator_index_1D(const T & width, const T & height)
	{
		uint64_t _width{};
		uint64_t _height{};
		utils::basic::store_in(_width, width);
		utils::basic::store_in(_height, height);
		return {_width, _height};
	}
}

namespace utils
{
	//导出常用的
	using utils::iterator::iterator_index;
	using utils::iterator::iterator_and_give_me_i;
	using utils::iterator::iterator_neighbour;
}
