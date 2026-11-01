#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <functional>
#include <utils/set.hpp>
#include <utils/Index2D.hpp>
#include <utils/iterator.hpp>
#include <utils/type_traits.hpp>
#include <utils/integer_type.hpp>

namespace utils::vector_function
{
	//把一个二维索引变为一维索引
	inline uint64_t get_index_1D(_In_ const uint64_t i, _In_ const uint64_t j, _In_ const uint64_t width)
	{
		return i + j * width;
	}
	//特化
	inline uint64_t get_index_1D(_In_ const utils::Index2D & index, _In_ const uint64_t width)
	{
		utils_assert(index.i >= 0);
		utils_assert(index.j >= 0);
		return get_index_1D(index.i, index.j, width);
	}

	//求交集
	template<typename T>
	void intersection(_Out_ std::vector<T> & result, _In_ const std::vector<T> & vec_0, _In_ const std::vector<T> & vec_1)
	{
		//简单检查
		static_assert(utils::is_arithmetic_v<T>);

		std::set<T> s0{vec_0.begin(), vec_0.end()};
		std::set<T> s1{vec_1.begin(), vec_1.end()};

		utils::set(s0).intersection_with(s1);
		result.assign(s0.begin(), s0.end());
	}

	//把数据块从一种类型转为另一种类型
	//note: 不做意义转换，仅仅相当于指针类型转换, i.e. double -> int 时，1个double会变为两个int
	//e.g.    std::vector<uint8_t>{0x01, 0x00, 0x00, 0x00}   ->  std::vector<uint32_t>{0x1}
	//e.g.    std::vector<int>{0x1}   ->  std::vector<uint32_t>{0x1}
	template<typename from_T, typename to_T>
	void change_type(_Out_ std::vector<to_T> & values, _In_ const std::vector<from_T> & data)
	{
		//简单检查
		static_assert(utils::is_arithmetic_v<from_T>, "");
		static_assert(utils::is_arithmetic_v<to_T>, "");
		utils_assert(data.size() * sizeof(from_T) % sizeof(to_T) == 0);

		values.assign((to_T*)&data[0], (to_T*)&data[0] + (data.size() * sizeof(from_T) / sizeof(to_T)));
	}

	//是否存在指定值
	template <typename T>
	bool exist(const std::vector<T> & vec, const T & value)
	{
		return std::find(vec.begin(), vec.end(), value) != vec.end();
	}

	//如果元素不存在则push_back
	template<typename T>
	void push_back_if_not_exist(std::vector<T> & vec, const T & value)
	{
		if (utils::vector_function::exist(vec, value))
			return;

		vec.push_back(value);
	}

	//删除重复元素
	template<typename T>
	void remove_duplicate(std::vector<T> & vec)
	{
		std::set<T> s{vec.begin(), vec.end()};
		vec.assign(s.begin(), s.end());
	}

	//制作一个3维数组，内容由外部提供
	//get_value：负责提供i,j,k元素的值
	//n1 n2 n3: 数组大小  对应于 vec[n1][n2][n3]
	template<typename T>
	void make_vector(_Out_ std::vector<std::vector<std::vector<T>>> & vec, uint64_t n1, uint64_t n2, uint64_t n3, std::function<T(uint64_t i, uint64_t j, uint64_t k)> get_value)
		{
			//简单检查
			static_assert(utils::is_arithmetic_v<T>, "");

			vec.resize(n1);
			for (auto i : utils::iterator_index(n1))
			{
				vec[i].resize(n2);
				for (auto j : utils::iterator_index(n2))
				{
					vec[i][j].resize(n3);
					for (auto k : utils::iterator_index(n3))
					{
						vec[i][j][k] = get_value(i, j, k);
					}
				}
			}
		}
}

namespace utils
{
	namespace inner
	{
		template<typename T>
		class vector : public std::vector<T>
		{
			//新加的函数
		public:
			//赋值
			void operator=(const std::set<T> & values)
			{
				this->assign(values.begin(), values.end());
				//return *this;
			}
			//是否存在指定值
			bool is_exist(const T & value) const
			{
				return utils::vector_function::exist(*this, value);
			}
			//特化
			bool is_exist_if(std::function<bool(const T & one)> meet_one_element) const
			{
				for (const auto & one : *this)
				{
					if (meet_one_element(one))
						return true;
				}
				return false;
			}
			//如果元素不存在则push_back
			void push_back_if_not_exist(const T & value)
			{
				return utils::vector_function::push_back_if_not_exist(*this, value);
			}
			//删除重复元素
			void remove_duplicate()
			{
				return utils::vector_function::remove_duplicate(*this);
			}
#if 0
#pragma region 太过于不常用
			//创建一个map, key由每个元素生成, value为元素的索引
			//example: std::vector{"a", "b", "c"} 可以返回 std::map{{strlen("a"), 0},{strlen("b"), 1},{strlen("c"), 2}}
			template<typename Key_T>
			void get_map_to_index(_Out_ std::map<Key_T, uint64_t> & map_to_index, _In_ std::function<Key_T(const T & value)> make_key_func) const
			{
				map_to_index.clear();
				for (const auto & e : utils::iterator_and_give_me_i(m_vector))
				{
					map_to_index[make_key_func(e.value)] = e.i;
				}
			}
#pragma region 类型转换
			//字符串转为字节数组, 不包括结尾的'\0'
			template<typename unused = std::enable_if_t<std::is_same_v<T, uint8_t>>>
			void from_type(_In_ const std::string & s)
			{
				m_vector.assign(s.begin(), s.begin() + s.length());
			}
			//转为std::string
			template<typename unused = std::enable_if_t<std::is_same_v<T, uint8_t>>>
			void to_type(_Out_ std::string & s) const
			{
				m_vector.assign(this->begin(), this->end());
			}
			//特化: 简便写法
			template<typename unused = std::enable_if_t<std::is_same_v<T, uint8_t>>>
			std::string to_string() const
			{
				std::string s;
				to_type(s);
				return s;
			}
#pragma endregion
#pragma endregion
#endif
			//逆序全部元素
			void reverse()
			{
				std::reverse(this->begin(), this->end());
			}
			//放入一些值
			template<typename T_container>
			void push_back_many(const T_container & values)
			{
				for (auto & one : values)
					this->push_back(one);
			}
			//特化
			template<typename T_container, typename = std::enable_if_t<std::is_rvalue_reference_v<T_container&&>>>
			void push_back_many(T_container && values)
			{
				for (auto & one : values)
					this->push_back(std::move(one));
			}
			//把指定元素放到末尾
			//return: 移动的元素个数
			uint64_t move_to_tail(std::function<bool(const T & one)> meet_one_element)
			{
				auto elements = remove_all(meet_one_element);
				uint64_t count = elements.size();
				push_back_many(std::move(elements));
				return count;
			}

			//检查是否有指定的前缀
			bool begin_with(const std::vector<T> & prefix) const
			{
				if (this->size() < prefix.size())
					return false;
				
				return memcmp(this->data(), prefix.data(), prefix.size()*sizeof(T)) == 0;
			}
			//特化
			template<typename = utils::enable_if_same<T, uint8_t>>
			bool begin_with(const std::string & text) const
			{
				std::vector<T> tmp;
				tmp.assign(text.begin(), text.end());
				return begin_with(tmp);
			}
			//弹出最后一个元素
			T pop_back()
			{
				//简单检查
				utils_assert(!this->empty());

				const auto value = std::move(last_one());
				this->resize(this->size()-1);
				return value;
			}
			//返回最后一个元素
			T & last_one()
			{
				//简单检查
				utils_assert(!this->empty());

				return (*this)[this->size()-1];
			}
			//特化
			const T & last_one() const
			{
				//简单检查
				utils_assert(!this->empty());

				return (*this)[this->size()-1];
			}
			//获得该元素的索引
			uint64_t get_index(const T & value) const
			{
				for(const auto & one : utils::iterator_and_give_me_i(*this))
				{
					if(&one.value == &value)
						return one.i;
				}
				utils_throw();
			}
			//删除指定元素
			void remove(const T & value)
			{
				std::vector<T>::erase(this->begin() + get_index(value));
			}
			//特化：此元素有且只有一个
			//return: 被删除的元素
			T remove_unique_if(std::function<bool(const T & one)> meet_one_element)
			{
				auto removed = remove_if(meet_one_element);
				utils_assert(removed.size() == 1);
				return std::move(removed[0]);
			}
			//特化：
			//return: 被删除的元素
			std::vector<T> remove_if(std::function<bool(const T & one)> meet_one_element)
			{
				std::vector<T> removed;
				for (auto it = this->begin(); it != this->end();)
				{
					if (meet_one_element(*it))
					{
						removed.push_back(std::move(*it));
						it = std::vector<T>::erase(it);
					}
					else
						it++;
				}
				return removed;
			}
			//是否已为排好序的状态
			bool is_sorted()
			{
				for(const auto & neighbour : utils::iterator_neighbour(*this))
				{
					if (neighbour.first < neighbour.second)
						continue;
					if (neighbour.second < neighbour.first)
						return false;
					continue;
				}
				return true;
			}
			struct s_hot_value_entry
			{
				T value;
				u8 count;
			};
			//寻找热值
			std::vector<s_hot_value_entry> count_hot_value() const
			{
				std::vector<s_hot_value_entry> hot_values;
				{
					std::map<T, u8> counts;
					for(const auto & one : *this)
					{
						counts[one]++;
					}

					for(const auto & one : counts)
					{
						hot_values.push_back({one.first, one.second});
					}

					std::sort(hot_values.begin(), hot_values.end(), [](const auto & a, const auto & b)
					{
						return a.count > b.count;
					});
				}
				return hot_values;
			}
		private:
			//由于其他容器的erase改为了remove，为了统一，这个也改为remove
			void erase()
			{
				utils_throw();
			}
		};
	}
	template<typename T>
	const inner::vector<T> & vector(const std::vector<T> & c)
	//需要指定返回类型，以避免返回的是值类型, 而不是一个引用
	{
		return (const inner::vector<T> &)c;
	}
	template<typename T>
	inner::vector<T> & vector(std::vector<T> & c)
	{
		return (inner::vector<T> &)c;
	}
}