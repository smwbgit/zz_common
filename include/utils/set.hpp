#pragma once

#include <set>
#include <utils/assert.hpp>

namespace utils
{
	namespace inner
	{
		template<typename T>
		class set : public std::set<T>
		{
		public:
			using std::set<T>::set;
			using std::set<T>::operator =;

		public:
			void operator-=(const std::set<T> & other)
			{
				for(const auto & one : other)
				{
					if(is_exist(one))
						this->erase(one);
				}
			}
			//是否存在某个值
			bool is_exist(const T & value) const 
			{
				return this->find(value) != this->end();
			}
#if 0
			//求并集
			void union_with(const std::set<T> & o)
			{
				for (const auto & v : o)
					insert(v);
			}
#endif
			//求交集
			std::set<T> get_intersection(const std::set<T> & other) const
			{
				//速度优化：如果this比other的元素个数少，就交换一下
				if (this->size() < other.size())
				{
					//转发
					return ((utils::inner::set<T>&)other).get_intersection(*this);
				}

				std::set<T> intersection;
				for (const auto & value : other)
				{
					if (this->is_exist(value))
						((utils::inner::set<T>&)(intersection)).insert_not_exist(value);	//todo 看看这里用不用直接插入，不做检查
				}
				return intersection;
			}
			//放入一些值
			template<typename T_container>
			void insert_many(const T_container & values)
			{
				for(const auto & value : values)
				{
					this->insert(value);
				}
			}
			//特化
			template<typename T_container>
			void insert_many_not_exist(const T_container & values)
			{
				for(const auto & value : values)
				{
					this->insert_not_exist(value);
				}
			}
			//序列化
			template<bool is_saving, typename T_serializer>
			void serialize_support(T_serializer & file)
			{
				file & (std::set<T>&)*this;
			}
			//插入一个不存在的
			void insert_not_exist(const T & value)
			{
				utils_assert(!is_exist(value));
				this->insert(value);
			}
			//获取第一个值
			const T & get_first_element() const
			{
				utils_assert(!this->empty());
				return *this->begin();
			}
			//特化
			T & get_first_element()
			{
				utils_assert(!this->empty());
				return *this->begin();
			}
			//移除指定元素
			void remove(const T & value)
			{
				//简单检查
				utils_assert(is_exist(value));

				std::set<T>::erase(value);
			}
			//特化：移除多个指定元素
			void remove_many(const std::set<T> & values)
			{
				for(const auto & value : values)
					remove(value);
			}
			//特化：移除交集部分
			void remove_intersection(const std::set<T> & values)
			{
				const auto intersection = get_intersection(values);
				remove_many(intersection);
			}
			//is_allow_duplicate: 是否允许出现重复项
			void assign_by_vector(const std::vector<T> & values, bool is_allow_duplicate = true)
			{
				this->clear();
				for(const auto & value : values)
				{
					if(is_allow_duplicate)
						this->insert(value);
					else
						this->insert_not_exist(value);
				}
			}
		private:
			//由于原始的erase，当元素不存在时不报错。为了避免混淆，把它隐藏掉。请改用remove()
			void erase()
			{
				utils_throw();
			}
		};
	}
	template<typename T>
	const inner::set<T> & set(const std::set<T> & c)
	{
		return (const inner::set<T> &)c;
	}
	template<typename T>
	inner::set<T> & set(std::set<T> & c)
	{
		return (inner::set<T> &)c;
	}
}