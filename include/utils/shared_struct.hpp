#pragma once

#include <memory>
#include "utils/virtual_destructor.hpp"
#include "utils/copy_on_write/shared_ptr.hpp"

/*
为类声明一个它的共享指针类型, 以s_sp_开头
note:
	生成类似如下结构:
	struct s_a;
	using s_sp_a = utils::copy_on_write::shared_ptr<s_a>;
	struct s_a
	{
		...
	};
*/
#define utils_shared_struct(name)	\
	struct s_##name;	\
	using s_sp_##name = utils::copy_on_write::shared_ptr<s_##name>;	\
	struct s_##name	\

//特化
#define utils_shared_child(name, parent)	\
	struct s_##name;	\
	using s_sp_##name = utils::copy_on_write::shared_ptr<s_##name>;	\
	struct s_##name : public s_##parent	\


#if 0   //not deleted only for their names 
#pragma region 工厂类的支持

//在工厂类中, 用于在最顶层类中, 声明一个纯虚函数
#define utils_factory_function_declaration(name) virtual s_sp_##name make_##name() const = 0;

//用于实现工厂类中的一个函数
//name: 用于函数名和返回类型
//object_type: 对象的真正类型
#define utils_factory_return_one_object(name, object_type)	\
	s_sp_##name make_##name() const { return utils::copy_on_write::make_shared<s_##object_type>(); }

#pragma endregion 
#endif