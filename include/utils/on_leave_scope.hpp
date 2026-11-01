#pragma once


#include "utils/CallWhenReturn.hpp"


//连接X和Y
#define utils_macro_combine1(X,Y) X##Y  // helper macro
#define utils_macro_combine(X,Y) utils_macro_combine1(X,Y)

//生成一个不会重名的名字，用于局部变量
#define utils_macro_unique_name(prefix)	utils_macro_combine(prefix, __COUNTER__)

//离开当前语句块时，调用指定语句
#define utils_on_leave_scope(statement) utils::CallWhenReturn utils_macro_unique_name(CallWhenReturn_guard){[&](){statement;}}
