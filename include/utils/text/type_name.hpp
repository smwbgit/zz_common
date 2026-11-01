#pragma once

#include <string>

//类型名称
namespace utils::text::type_name
{
	//获得类型的名字
	template<typename T>
	std::string get_type_name();
	//特化
	template<> inline std::string get_type_name<int8_t  >()	{ return "int8_t"  ; }
	template<> inline std::string get_type_name<int16_t >()	{ return "int16_t" ; }
	template<> inline std::string get_type_name<int32_t >()	{ return "int32_t" ; }
	template<> inline std::string get_type_name<int64_t >()	{ return "int64_t" ; }
	template<> inline std::string get_type_name<uint8_t >()	{ return "uint8_t" ; }
	template<> inline std::string get_type_name<uint16_t>()	{ return "uint16_t"; }
	template<> inline std::string get_type_name<uint32_t>()	{ return "uint32_t"; }
	template<> inline std::string get_type_name<uint64_t>()	{ return "uint64_t"; }
	template<> inline std::string get_type_name<float>()	{ return "float";    }
	template<> inline std::string get_type_name<double>()	{ return "double";   }
	template<> inline std::string get_type_name<bool>()		{ return "bool";     }
}