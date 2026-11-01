#pragma once

#include <string>
#include <utils/map.hpp>

namespace utils::debug
{
	//用于给一个对象增加注释
	//用途举例：当调试时，方便在调试器里，通过窗口对象，确定是哪个窗口
	class DebugNote
	{
	private:
		std::map<std::u8string, std::u8string> m_notes;
	public:
		//添加一条 key不存在 的注释
		void add_not_existed(const std::u8string & key, const std::u8string & value)
		{
			utils::map(m_notes).insert_not_exist(key, value);
		}
	};
}