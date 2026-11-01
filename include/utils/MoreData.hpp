#pragma once

#include <string>
#include <utils/map.hpp>

namespace utils
{
	//用于结构体成员的扩展
	//example:
	//	struct s_module : public utils::MoreData
	//	{
	//		uint64_t m_base{};
	//		uint64_t m_size{};
	//		std::u8string m_name;
	//		std::u8string m_full_name;
	//	};
	class MoreData
	{
	private:
		std::map<std::u8string, std::unique_ptr<std::any>> m_more_data;

	public:
		//note: 需要已经存在
		template<typename T>
		T & get_more_data(const std::u8string & name)
		{
			auto & one = utils::map(m_more_data).get_existed_value(name);
			return std::any_cast<T&>(*one);
		}
		template<typename T>
		void add_more_data(const std::u8string & name, T&& value)
		{
			auto one = std::make_unique<std::any>(std::forward<T>(value));
			utils::map(m_more_data).insert_not_exist(name, std::move(one));
		}
		//是否存在指定数据
		bool has_more_data(const std::u8string & name)
		{
			return utils::map(m_more_data).is_exist(name);
		}
	};
}