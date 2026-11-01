#pragma once

#include <string>

namespace utils::command_line
{
	//生成一条命令的完整字符串
	class OneCommand
	{
	private:
		std::string m_cmd;
		bool m_is_first{true};
	public:
		//向最后添加一个字段
		//is_need_add_double_quote: 是否需要用双引号把该字段括起来
		//note: 因为第一个字段通常是可执行文件的位置，所以会将其括起来，不论is_need_add_double_quote如何设置
		OneCommand & add(const std::string & one_part, bool is_need_add_double_quote = false)
		{
			//准备参数
			if (m_is_first)
			{
				m_is_first = false;
				is_need_add_double_quote = true;
			}

			//添加字段
			{
				if (is_need_add_double_quote)
					m_cmd += '\"';

				m_cmd += one_part;

				if (is_need_add_double_quote)
					m_cmd += '\"';

				m_cmd += ' ';
			}
			return *this;
		}
		//获取字符串
		operator const std::string &() const
		{
			return m_cmd;
		}
	};
}
