#pragma once

#include <exception>
#include <string>
#include <functional>

//修复
namespace utils::restore
{
}

/*按块存在的列表，比如
a 0
a 1
a 2
b 0
b 1
b 2
c 0
c 1
c 2
就是按a,b,c划分成3块
*/
namespace utils::restore::list::blocks
{
	//优先使用和上一行一样的
	class Restore
	{
	private:
		std::any m_last;
	public:
		//每遇到一行，就调用一次。传入所有可能的值，返回时，会把最可能的放在第一个
		template<typename T, typename T_key>
		void sort(std::vector<T> & vec, std::function<T_key(const T &)> get_key)
		{
			//简单检查
			utils_assert(!vec.empty());
			utils_assert(m_last.has_value() == false || m_last.type() == typeid(T_key));

			if (vec.size() > 1 && m_last.has_value())
			{
				for (auto & one : vec)
				{
					if (get_key(one) == std::any_cast<const T_key &>(m_last))
					{
						std::swap(one, vec[0]);
						break;
					}
				}
			}

			m_last = get_key(vec[0]);
		}
	};
}