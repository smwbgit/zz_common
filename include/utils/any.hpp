#pragma once


#include <unordered_map>
#include <typeindex>
#include <any>
#include <functional>


namespace utils::any_function
{
	inline void get_integer(uint64_t& val, bool& is_unsigned, uint64_t& size, const std::any& _any)
	{
		struct s_type
		{
			bool is_unsigned;
			uint64_t size;
			std::function<void(uint64_t& val, const std::any& _any)> get_val;
		};
#define table_entry(T, is_unsigned) {std::type_index(typeid(T)), {is_unsigned, sizeof(T), [](uint64_t& val, const std::any& _any) {val = std::any_cast<T>(_any); }}}
		static const std::unordered_map<std::type_index, s_type> table = {
			table_entry(int8_t, false),
			table_entry(int16_t, false),
			table_entry(int32_t, false),
			table_entry(int64_t, false),
			table_entry(uint8_t, true),
			table_entry(uint16_t, true),
			table_entry(uint32_t, true),
			table_entry(uint64_t, true),
		};
#undef table_entry

		auto& type = _any.type();
		is_unsigned = table.at(std::type_index(type)).is_unsigned;
		size = table.at(std::type_index(type)).size;
		table.at(std::type_index(type)).get_val(val, _any);
	}
}