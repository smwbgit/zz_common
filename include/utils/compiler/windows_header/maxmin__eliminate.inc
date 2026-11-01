//不能嵌套使用
#ifdef max
#define utils_redefine_max
#undef max
#endif
#ifdef min
#define utils_redefine_min
#undef min
#endif