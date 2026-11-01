#pragma once


namespace utils
{
	namespace inner
	{
		//一个三维坐标点
		template<typename T>
		struct Basic_Point3D
		{
			T x{};
			T y{};
			T z{};

			Basic_Point3D() = default;
			Basic_Point3D(T x, T y, T z) : x{x}, y{y}, z{z}
			{
			}
			template<typename Other_T>
			explicit operator Basic_Point3D<Other_T> () const
			{
				static_assert(std::is_floating_point<Other_T>::value, "");

				return Basic_Point3D<Other_T>{(Other_T)x, (Other_T)y, (Other_T)z};
			}
			Basic_Point3D operator*(const T val) const
			{
				Basic_Point3D re = *this;
				re *= val;
				return re;
			}
			Basic_Point3D operator/(const T val) const
			{
				Basic_Point3D re = *this;
				re /= val;
				return re;
			}
			Basic_Point3D & operator *=(const T val)
			{
				x *= val;
				y *= val;
				z *= val;
				return *this;
			}
			Basic_Point3D & operator /=(const T val)
			{
				x /= val;
				y /= val;
				z /= val;
				return *this;
			}
			Basic_Point3D operator+(const Basic_Point3D & o) const
			{
				Basic_Point3D re = *this;
				re += o;
				return re;
			}
			Basic_Point3D operator-(const Basic_Point3D & o) const
			{
				Basic_Point3D re = *this;
				re -= o;
				return re;
			}
			Basic_Point3D & operator +=(const Basic_Point3D & o)
			{
				x += o.x;
				y += o.y;
				z += o.z;
				return *this;
			}
			Basic_Point3D & operator -=(const Basic_Point3D & o)
			{
				x -= o.x;
				y -= o.y;
				z -= o.z;
				return *this;
			}
			bool operator ==(const Basic_Point3D & o) const
			{
				return x == o.x && y == o.y && z == o.z;
			}

			//序列化支持
			template<typename T>
			void serialize(T& file, bool is_serialize)
			{
				file & x;
				file & y;
				file & z;
			}
		};

		template<typename T>
		Basic_Point3D<T> operator*(const T value, const Basic_Point3D<T> & pt)
		{
			return pt * value;
		}

	}

	//一个三维坐标点  float版
	using Point3D = inner::Basic_Point3D<float>;
	//一个三维坐标点  double版
	using Point3Dd = inner::Basic_Point3D<double>;
}