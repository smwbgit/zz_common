#pragma once

#include <utils/throw.hpp>

//断言
#define utils_assert(A)  do {if (bool(A) == false) utils_throw_with_message("utils_assert expected:  " #A); }while(0)

//特化：当失败时，直接退出程序。通常用于析构函数中的检查
#define utils_assert_abort(A)  do {if (bool(A) == false) abort(); }while(0)