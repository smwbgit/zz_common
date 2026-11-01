#pragma once

#include <algorithm>
#include <map>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>

namespace utils
{
	namespace inner
	{
		template<typename T1, typename T2>
		class map : public std::map<T1, T2>
		{
		public:
			using std::map<T1, T2>::map;
			using std::map<T1, T2>::operator =;
		
			//构造
			map() = default;
			map(const std::map<T1, T2> & o)
				: std::map<T1, T2>{o}
			{
			}
			//赋值
			map & operator =(const std::map<T1, T2> & o)
			{
				*this = map(o);		//todo: 这里似乎会复制两次
				return *this;
			}
			//序列化
			template<bool is_saving, typename T_serializer>
			void serialize_support(T_serializer & file)
			{
				auto & map = (std::map<T1, T2> &)*this;
				file & map;
			}
			//索引
			T2 & operator[](const T1 & key)
			{
				auto & _map = (std::map<T1, T2> &)*this;
				return _map[key];
			}
			//特化
			const T2 & operator[](const T1 & key) const
			{
				auto & _map = (std::map<T1, T2> &)*this;
				return _map[key];
			}
			//如果不存在则插入
			void insert_if_not_exist(const T1 & key, const T2 & value)
			{
				if(!is_exist(key))
					(*this)[key] = value;
			}
			//插入一个不存在的
			void insert_not_exist(const T1 & key, const T2 & value)
			{
				utils_assert(this->contains(key) == false);
				(*this)[key] = value;
			}
			//特化
			void insert_not_exist(const T1 & key, T2 && value)
			{
				utils_assert(this->contains(key) == false);
				(*this)[key] = std::move(value);
			}
			//获取一个存在的
			const T2 & get_existed_value(const T1 & key) const
			{
				utils_assert(this->contains(key));
				return this->at(key);
			}
			//特化
			T2 & get_existed_value(const T1 & key)
			{
				utils_assert(this->contains(key));
				return this->at(key);
			}
			//获取一个可选的key。如果不存在，则无操作
			//return: 是否存在
			bool get_optional_value(const T1 & key, T2 & value)
			{
				auto iter = this->find(key);
				if (iter == this->end())
					return false;

				value = iter->second;
				return true;
			}
			//返回一个按value排序的(key, value)数组
			template<typename CompareFunc_T>
			void get_sorted_vector(_Out_ std::vector<std::tuple<T1, T2>> & vec, CompareFunc_T compare_func) const
			{
				//复制全部数据
				vec.clear();
				{
					for (const auto & v : *this)
					{
						vec.push_back({v.first, v.second});
					}
				}

				//按value排序
				using tuple_t = std::tuple<T1, T2>;
				std::sort(vec.begin(), vec.end(), [compare_func](const tuple_t & t1, const tuple_t & t2)
				{
					const auto & value1 = std::get<1>(t1);
					const auto & value2 = std::get<1>(t2);

					return compare_func(value1, value2);
				});
			}
			//是否存在指定的key
			bool is_exist(const T1 & key) const 
			{
				return this->contains(key);
			}
			//获得所有的key
			std::vector<T1> get_all_key() const
			{
				std::vector<T1> keys;
				for(const auto & one : *this)
				{
					keys.push_back(one.first);
				}
				return keys;
			}
			//移除指定元素
			//note: 元素需存在
			void remove(const T1 & key)
			{
				//简单检查
				utils_assert(is_exist(key));

				std::map<T1, T2>::erase(key);
			}
			//由一个vector变为map
			//note: 由于假设T1和T2比较小，所以get_key的返回值直接使用T1，而不是T1的引用。也由于lambda函数的书写要特意注明返回的是引用，感觉不太方便。也由于编译器的报错信息，会不给出具体是哪一行编译没通过，在lambda没注明返回类型是引用时。
			void assign_by_vector(const std::vector<T2> & vec, std::function<T1(const T2 & one)> get_key)
			{
				this->clear();
				for(const auto & one : vec)
				{
					insert_not_exist(get_key(one), one);
				}
			}
		public:
			template<bool is_const>
			class s_iterator
			{
			private:
				using iterator = utils::select_type<is_const, typename map<T1,T2>::const_iterator, typename map<T1,T2>::iterator>;
				using second_value_t = utils::add_const<is_const, T2>;
			private:
				iterator m_iterator;
			public:
				s_iterator(iterator && _iterator)
					: m_iterator{std::move(_iterator)}
				{
				}
				second_value_t & operator*()
				{
					auto & value = *m_iterator;
					return value.second;
				}
				void operator++()
				{
					m_iterator++;
				}
				bool operator !=(const s_iterator<is_const> & o) const
				{
					return m_iterator != o.m_iterator;
				}
			};
			template<bool is_const>
			class s_iterator_range
			{
			private:
				using map_t = utils::add_const<is_const, map<T1,T2>>;
				using iterator = s_iterator<is_const>;
			private:
				map_t & m_this{};
			public:
				s_iterator_range(map_t & _this)
					: m_this{_this}
				{
				}
				iterator begin()
				{
					return iterator{m_this.begin()};
				}
				iterator end()
				{
					return iterator{m_this.end()};
				}
			};
		public:
			auto iterator_seconds()
			{
				return s_iterator_range<false>{*this};
			}
			auto iterator_seconds() const
			{
				return s_iterator_range<true>{*this};
			}
		private:
			//由于原始的erase，当元素不存在时不报错。为了避免混淆，把它隐藏掉。请改用remove()
			void erase()
			{
				utils_throw();
			}
		};
	}
	template<typename T1, typename T2>
	const inner::map<T1,T2> & map(const std::map<T1,T2> & c)
	{
		return (const inner::map<T1,T2> &)c;
	}
	template<typename T1, typename T2>
	inner::map<T1,T2> & map(std::map<T1,T2> & c)
	{
		return (inner::map<T1,T2> &)c;
	}
}