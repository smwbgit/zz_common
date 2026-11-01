#pragma once

#include <string>
#include "utils/assert.hpp"

//对于writer,可选的附加功能
namespace utils::writer::optional_function
{
	//功能: 插入缩进
	template<typename BaseClass_T>
	class TabInsert : public BaseClass_T
	{
	public:
		using BaseClass_T::BaseClass_T;

	public:
		class s_tab_insert
		{
		private:
			BaseClass_T* m_this{};
			int64_t m_tab_count{};
			std::string m_tab_text{"\t"};
		public:
			s_tab_insert(BaseClass_T* _this)
				: m_this{_this}
			{
			}
			//改变缩进位置
			//change: 正数表示缩进变大, 负数表示缩进变小
			void change_tab_count(int64_t change)
			{
				m_tab_count += change;

				//简单检查
				utils_assert(m_tab_count >= 0);
			}
			//获取缩进位置
			uint64_t get_tab_count()
			{
				return (uint64_t) m_tab_count;
			}
			//设置tab的表示形式.
			//e.g.: "    " 或者 "\t" etc.
			void set_tab_text(const std::string & tab_text)
			{
				m_tab_text = tab_text;
			}
			//写入一行, 前边自动增加相应数量的tab
			void write_one_line(const std::string & text)
			{
				//添加tab
				std::string _text;
				for (int64_t i = 0; i < m_tab_count; i++)	//todo: 把for循环改为使用准备好的前导字符串
				{
					_text += m_tab_text;
				}
				_text += text;

				//转发
				m_this->write_one_line(_text);
			}
		};
		
	public:
		s_tab_insert tab_insert{this};
	};
}