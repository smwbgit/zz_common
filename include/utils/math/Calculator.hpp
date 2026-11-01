#pragma once

#include <array>
#include "utils/assert.hpp"

namespace utils::math
{
	//在此类中进行各种运算
	class Calculator
	{
	public:
		//minimal_error: 容许的最小误差
		Calculator(float minimal_error)
			: m_minimal_error{minimal_error}
		{
			utils_assert(m_minimal_error > 0);
		}
		
#pragma region 多项式求值
		//计算一元二次多项式的值
		//note: y = coefficients[0] * x * x + coefficients[1] * x + coefficients[2]
		void calc_one_variable_quadratic_polynomial(_Out_ double & value, _In_ const std::array<double, 3> & coefficients, _In_ const double x)
		{
			value = coefficients[0] * x * x + coefficients[1] * x + coefficients[2];
		}
#pragma endregion

#pragma region 行列式
		//计算三阶行列式
		void calc_determinant(_Out_ double & value,
				_In_ const double a11, _In_ const double a12, _In_ const double a13,
				_In_ const double a21, _In_ const double a22, _In_ const double a23,
				_In_ const double a31, _In_ const double a32, _In_ const double a33)
		{
			//参见：https://jingyan.baidu.com/article/e75aca85003645142edac60f.html

			value = a11*a22*a33 + a12*a23*a31 + a13*a21*a32
				- a13*a22*a31 - a12*a21*a33 - a11*a23*a32;
		}
#pragma endregion

#pragma region 解方程组
		//解3元1次方程组
		//note:
		// a11*x + a12*y + a13*z = b1
		// a21*x + a22*y + a23*z = b2
		// a31*x + a32*y + a33*z = b3
		//relations：方程组的解
		void solve_three_variable_linear_equations(_Out_ std::array<double, 3> & relations,
				_In_ const double a11, _In_ const double a12, _In_ const double a13, _In_ const double b1,
				_In_ const double a21, _In_ const double a22, _In_ const double a23, _In_ const double b2,
				_In_ const double a31, _In_ const double a32, _In_ const double a33, _In_ const double b3)
		{
			//参见：https://jingyan.baidu.com/article/e75aca85003645142edac60f.html

			double D{};
			calc_determinant(D,
				a11, a12, a13,
				a21, a22, a23,
				a31, a32, a33);

			double D1{};
			calc_determinant(D1,
				b1, a12, a13,
				b2, a22, a23,
				b3, a32, a33);

			double D2{};
			calc_determinant(D2,
				a11, b1, a13,
				a21, b2, a23,
				a31, b3, a33);

			double D3{};
			calc_determinant(D3,
				a11, a12, b1,
				a21, a22, b2,
				a31, a32, b3);

			//简单检查
			utils_assert(D != 0);	//看看这里怎么写合适

			relations[0] = D1 / D;
			relations[1] = D2 / D;
			relations[2] = D3 / D;
		}
#pragma endregion

	protected:
		float m_minimal_error{};
	};

}