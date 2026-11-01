#pragma once

#include <string>
#include <any>
#include <utils/value/EventedValue.hpp>
#include <utils/map.hpp>

namespace utils::value
{
	//提供名字到utils::value::EventedValue的映射
	//相当于 std::map<std::u8string /*name*/, utils::value::EventedValue<不同的T> /*value*/>
	class EventedValueManager
	{
	private:
		std::map<std::u8string, std::any> m_values;
		std::map<std::u8string, void*> m_only_pointer_values;
	public:
		template<typename T>
		EventedValue<T> & get_existed_value(const std::u8string & name)
		{
			if(utils::map(m_values).is_exist(name))
			{
				auto & any = utils::map(m_values).get_existed_value(name);
				return std::any_cast<EventedValue<T>&>(any);
			}
			else
			{
				auto pointer = utils::map(m_only_pointer_values).get_existed_value(name);
				return *(EventedValue<T>*)pointer;
			}
		}
		template<typename T>
		void create_only_pointer_value(const std::u8string & name, EventedValue<T> & value)
		{
			//简单检查
			utils_assert(!utils::map(m_values).is_exist(name));

			utils::map(m_only_pointer_values).insert_not_exist(name, &value);
		}
	};
}