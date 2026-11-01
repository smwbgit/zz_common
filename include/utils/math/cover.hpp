#pragma once

//重叠
namespace utils::math::cover
{
#if 0	//请使用 Range1D 或 Range2D 中的 is_overlap 方法
	//两个线段是否重叠
	template<typename T>
	inline bool is_overlap(T begin_1, T size_1, T begin_2, T size_2)
	{
		//简单检查
		static_assert(utils::is_integral_v<T>);

		//todo 检查下边的加法是否溢出
		if(begin_1 + size_1 <= begin_2)	return false;	//1在2的左边
		if(begin_1 >= begin_2 + size_2) return false;	//1在2的右边
		return true;
	}
#endif
}