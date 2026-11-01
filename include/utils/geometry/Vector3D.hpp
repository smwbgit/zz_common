#pragma once

#include "utils/Point3D.hpp"

namespace utils::geometry
{
	namespace inner
	{
		//一个空间向量
		template<typename T>
		class Basic_Vector3D
		{
		public:
			T x{};
			T y{};
			T z{};

			Basic_Vector3D() = default;
			Basic_Vector3D(T x, T y, T z) : x{x}, y{y}, z{z} { }
			Basic_Vector3D(const utils::inner::Basic_Point3D<T> & point)
			{
				x = point.x;
				y = point.y;
				z = point.z;
			}

			//取模
			T length() const
			{
				return sqrt(x*x+y*y+z*z);
			}
		
			//点乘
			T dot_product(const Basic_Vector3D & o) const
			{
				return x*o.x + y*o.y + z*o.z;
			}
		};
	}
	
	using Vector3D = inner::Basic_Vector3D<float>;
	using Vector3Dd = inner::Basic_Vector3D<double>;
}
