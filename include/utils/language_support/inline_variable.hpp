#pragma once

//相当于C++17的inline
#if 0
#if defined(utils_on_windows)
#	define inline_variable __declspec(selectany) extern
#elif defined(utils_on_linux)
#	define inline_variable __attribute__((selectany)) extern
#else
#	error
#endif
#else
#define inline_variable inline
#endif
