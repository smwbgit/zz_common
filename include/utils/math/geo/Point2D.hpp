#pragma once

#include "utils/math/algebra.hpp"

namespace utils
{
	namespace inner
	{
		//一个二维坐标点
		template<typename T>
		struct Basic_Point2D
		{
			T x{};
			T y{};

			Basic_Point2D() = default;
			Basic_Point2D(const Basic_Point2D &) = default;
			Basic_Point2D(Basic_Point2D &&) = default;
			Basic_Point2D(T x, T y)
				: x{x}, y{y}
			{
			}
			Basic_Point2D(_In_ const T values[2])
				: x{values[0]}, y{values[1]}
			{
			}
			template<typename Other_T>
			explicit operator Basic_Point2D<Other_T> () const
			{
				static_assert(std::is_floating_point<Other_T>::value, "");

				return Basic_Point2D<Other_T>{(Other_T)x, (Other_T)y};
			}

			Basic_Point2D operator*=(const T val)
			{
				x *= val;
				y *= val;
				return *this;
			}
			Basic_Point2D operator*(const T val) const
			{
				Basic_Point2D re = *this;
				re *= val;
				return re;
			}
			Basic_Point2D operator+=(const Basic_Point2D& o)
			{
				x += o.x;
				y += o.y;
				return *this;
			}
			Basic_Point2D operator+(const Basic_Point2D& o) const
			{
				Basic_Point2D re = *this;
				re += o;
				return re;
			}
			Basic_Point2D operator-=(const Basic_Point2D& o)
			{
				x -= o.x;
				y -= o.y;
				return *this;
			}
			Basic_Point2D operator-(const Basic_Point2D& o) const
			{
				Basic_Point2D re = *this;
				re -= o;
				return re;
			}
			bool operator ==(const Basic_Point2D & o) const
			{
				return x == o.x && y == o.y;
			}
			//获取长度
			T length() const
			{
				return utils::math::algebra::sqrt((double)x*x + (double)y*y);	//todo: 看看这里的double怎么写
			}
		};
	}

	using Point2D = inner::Basic_Point2D<float>;
	using Point2Dd = inner::Basic_Point2D<double>;
}