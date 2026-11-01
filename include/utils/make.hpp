#pragma once

#include <utils/data.hpp>
#include <utils/string.hpp>

namespace utils
{
	//根据一种类型，生成另一种
	template<typename T>
	T make(const utils::data & data);
	
	template<>
	utils::string make<utils::string>(const utils::data & data)
	{
		utils::string re;
		re.assign((const char*)data.addr(), data.size());
		return re;
	}
	template<>
	std::wstring make<std::wstring>(const utils::data & data)
	{
		//简单检查
		utils_assert(data.size() % 2 == 0);

		std::wstring re;
		re.assign((const wchar_t*)data.addr(), data.size()/2);
		return re;
	}
}