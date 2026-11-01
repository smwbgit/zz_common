#pragma once

#include "utils/geometry/Vector3D.hpp"
#include "utils/assert.hpp"
#include "utils/Point2D.hpp"
#include "utils/math/Calculator.hpp"

namespace utils::geometry
{
	//在此类中进行各种运算
	class Calculator : public utils::math::Calculator
	{
	public:
		//minimal_error: 容许的最小误差
		Calculator(float minimal_error)
			: utils::math::Calculator{minimal_error}
		{
			utils_assert(m_minimal_error > 0);
		}

		//获得两向量间的夹角
		//cos: 夹角的cos值
		void get_angle_with_vector(_Out_ float & cos, const Vector3D & v1, const Vector3D & v2)
		{
			float length1 = v1.length();
			float length2 = v2.length();
			utils_assert(length1 >= m_minimal_error);
			utils_assert(length2 >= m_minimal_error);
			cos = v1.dot_product(v2) / length1 / length2;
		}
		
#pragma region 直线插值
#if 0
		//给定一个直线，通过插值算出其上的一个点。   通过z 计算 xyz
		void get_point_on_straight_line__z_to_xyz(
			_Out_ utils::Point3D & point,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const float z)
		{
			//简单检查
			utils_assert(control_points.size() == 2);
			utils_assert(fabs(control_points[0].z - control_points[1].z) >= m_minimal_error);
			
			point.x = (float)(control_points[0].x - (control_points[0].x - control_points[1].x)  * (control_points[0].z - z)   /  (control_points[0].z - control_points[1].z));
			point.y = (float)(control_points[0].y - (control_points[0].y - control_points[1].y)  * (control_points[0].z - z)   /  (control_points[0].z - control_points[1].z));
			point.z = z;
		}
#endif
		//给定一个直线，通过插值算出其上的一个点。   通过z 计算 xyz
		void get_point_on_straight_line__z_to_xyz(
			_Out_ utils::Point3D & point,
			_In_ const utils::Point3Dd & control_point_0,
			_In_ const utils::Point3Dd & control_point_1,
			_In_ const float z)
		{
			//简单检查
			utils_assert(fabs(control_point_0.z - control_point_1.z) >= m_minimal_error);
			
#if 1
			point.x = (float)(control_point_0.x - (control_point_0.x - control_point_1.x)  * (control_point_0.z - z)   /  (control_point_0.z - control_point_1.z));
			point.y = (float)(control_point_0.y - (control_point_0.y - control_point_1.y)  * (control_point_0.z - z)   /  (control_point_0.z - control_point_1.z));
			point.z = z;
#elif 0
			point.x = (float)control_point_0.x - ((float)control_point_0.x - (float)control_point_1.x)  * ((float)control_point_0.z - z)   /  ((float)control_point_0.z - (float)control_point_1.z);
			point.y = (float)control_point_0.y - ((float)control_point_0.y - (float)control_point_1.y)  * ((float)control_point_0.z - z)   /  ((float)control_point_0.z - (float)control_point_1.z);
			point.z = z;
#else
			point.x = (float)control_point_1.x - ((float)control_point_1.x - (float)control_point_0.x)  * ((float)control_point_1.z - z)   /  ((float)control_point_1.z - (float)control_point_0.z);
			point.y = (float)control_point_0.y - ((float)control_point_0.y - (float)control_point_1.y)  * ((float)control_point_0.z - z)   /  ((float)control_point_0.z - (float)control_point_1.z);
			point.z = z;
#endif
		}

#pragma endregion
#pragma region 曲线拟合
#pragma region 多项式
		//获得拟合后的多项式系数
		//note: coefficients[0] * x * x + coefficients[1] * x + coefficients[2] = y
		//points: 曲线上的点
		void get_polynomial_fitting_coefficients(_Out_ std::array<double, 3> & coefficients, _In_ const std::array<utils::Point2Dd, 3> & points)
		{
			//coefficients[0] * x * x + coefficients[1] * x + coefficients[2] = y;
			double a11 = points[0].x * points[0].x;
			double a12 = points[0].x;
			double a13 = 1;
			double b1 = points[0].y;
			double a21 = points[1].x * points[1].x;
			double a22 = points[1].x;
			double a23 = 1;
			double b2 = points[1].y;
			double a31 = points[2].x * points[2].x;
			double a32 = points[2].x;
			double a33 = 1;
			double b3 = points[2].y;

			solve_three_variable_linear_equations(coefficients,
				a11, a12, a13, b1,
				a21, a22, a23, b2,
				a31, a32, a33, b3
				);
		}

#pragma endregion
#pragma endregion

#pragma region 贝塞尔曲线插值
		//给定一个贝塞尔曲线，通过t精确算出一个点
		void get_point_on_bezier_curve(
			_Out_ utils::Point3Dd & point,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const double t)
		{
#if 0
			//简单检查
			utils_assert(control_points.size() >= 3);

			//迭代，直到只剩一个点
			std::vector<utils::Point3Dd> points = control_points;
			while (points.size() > 1)
			{
				std::vector<utils::Point3Dd> _points;
				for (auto i : utils::iterator_index(points.size() - 1))
				{
					const auto & pt1 = points[i];
					const auto & pt2 = points[i+1];
					_points.push_back(pt1+(pt2-pt1)*t);
				}
				points = std::move(_points);
			}

			//给出
			point = points[0];
#else
			if (control_points.size() == 4)
			{
				point =  (1-t)*(1-t)*(1-t) * control_points[0]
					+ 3*t*(1-t)*(1-t) * control_points[1]
					+ 3*t*t*(1-t) * control_points[2]
					+ t*t*t * control_points[3];
			}
			else
				utils_throw();	//todo: 继续补充
#endif

		}
		//给定一个贝塞尔曲线，通过插值算出其上的一个点。   通过z 计算 xyz
		void get_point_on_bezier_curve__z_to_xyz(
			_Out_ utils::Point3D & point,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const float z)
		{
			auto calc = [&](const double t) -> utils::Point3Dd
			{
				utils::Point3Dd pt;
				get_point_on_bezier_curve(pt, control_points, t);
				return pt;
			};

#if 0
			std::function<void(double t, double t_min, double t_max, const utils::Point3D point_min, const utils::Point3D point_max)> look_this_value =
				[&](double t, double t_min, double t_max, const utils::Point3D point_min, const utils::Point3D point_max)
			{
				auto _pt = calc(t);

				//如果精度达到，就返回
				if (fabs(_pt.z - z) <= z_result_precision)
				{
					point = _pt;
					return;
				}
				
				//看看离哪边近，向那边迭代
				if (fabs(_pt.z - point_min.z) < fabs(_pt.z - point_max.z))
				{
					look_this_value((t + t_min) / 2, t_min, t, point_min, _pt);
				}
				else
				{
					look_this_value((t + t_max) / 2, t, t_max, _pt, point_max);
				}
			};
#endif

			double t = 0.5;
			double t_min = 0;
			double t_max = 1;
			utils::Point3Dd point_min = calc(0);
			utils::Point3Dd point_max = calc(1);
			utils_assert(fabs(point_min.z - point_max.z) >= m_minimal_error);
			const bool is_sign = point_min.z < point_max.z;
			utils_assert(is_sign ? (point_min.z <= z && z <= point_max.z) : (point_min.z >= z && z >= point_max.z));
			while(1)
			{
				auto _pt = calc(t);

				//如果精度达到，就返回
//				if (fabs(_pt.z - z) <= z_result_precision)
				if ((float)_pt.z == z)
				{
					point.x = (float)_pt.x;
					point.y = (float)_pt.y;
					point.z = (float)_pt.z;
					return;
				}
				
				//看看离哪边近，向那边迭代
				if (is_sign ? _pt.z < z : _pt.z > z)
//				if (fabs(_pt.z - point_min.z) < fabs(_pt.z - point_max.z))
				{
					t_min = t;
					t = (t_min + t_max) / 2;
					point_min = calc(t_min);
				}
				else
				{
					t_max = t;
					t = (t_min + t_max) / 2;
					point_max = calc(t_max);
				}
			}
		}
#pragma endregion


#pragma region 待挪出
#if 0
		//给定一个折线，通过插值算出其上的一个点。   通过z 计算 xyz
		void get_point_on_polygonal_line__z_to_xyz(
			_Out_ utils::Point3D & point,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const float z)
		{
			//简单检查
			utils_assert(control_points.size() >= 2);
			for (auto i : utils::iterator_index(control_points.size() - 1))
				utils_assert(fabs(control_points[i].z - control_points[i+1].z) >= m_minimal_error);
			
#if 1
			auto use_normal = [&](utils::Point3D & point)
			{
				//找出需要的那根直线
				bool is_sign = control_points[0].z < control_points[1].z;
				for (auto i : utils::iterator_index(control_points.size() - 2))
				{
					if (is_sign ? z <= control_points[i+1].z : z >= control_points[i+1].z)
					{
						get_point_on_straight_line__z_to_xyz(point, control_points[i], control_points[i+1], z);
						return;
					}
				}
				//使用最后一根
				get_point_on_straight_line__z_to_xyz(point, control_points[control_points.size() - 2], control_points[control_points.size() - 1], z);
			};

			auto use_first = [&](utils::Point3D & point)
			{
				//使用第一根
				get_point_on_straight_line__z_to_xyz(point, control_points[0], control_points[1], z);
			};

			auto use_last = [&](utils::Point3D & point)
			{
				//使用最后一根
				get_point_on_straight_line__z_to_xyz(point, control_points[control_points.size() - 2], control_points[control_points.size() - 1], z);
			};

//			utils::Point3D normal;
//			utils::Point3D first;
//			use_normal(normal);
//			use_first(first);
			use_last(point);

//			double t = 1 - (1. * control_points[0].z - z) / (1. * control_points[0].z - control_points[1].z);
//			point.x = normal.x + t * (first.x - normal.x);
//			point.y = normal.y + t * (first.y - normal.y);
//			point.z = normal.z;
#endif
		}
#endif
#if 0
		//特化:  算出一系列点
		void get_points_on_polygonal_line__z_to_xyz(
			_Out_ std::vector<utils::Point3D> & points,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const std::vector<float> & z_values)
		{
			for (const auto z : z_values)
			{
				utils::Point3D pt;
				get_point_on_polygonal_line__z_to_xyz(pt, control_points, z);
				points.push_back(pt);
			}
		}
#endif


		//
		
		//特化:  算出一系列点
		void get_points_on_bezier_curve__z_to_xyz(
			_Out_ std::vector<utils::Point3D> & points,
			_In_ const std::vector<utils::Point3Dd> & control_points,
			_In_ const std::vector<float> & z_values)
		{
			for (const auto z : z_values)
			{
				utils::Point3D pt;
				get_point_on_bezier_curve__z_to_xyz(pt, control_points, z);
				points.push_back(pt);
			}
		}
#pragma endregion
	private:
//		float m_minimal_error{};
//		utils::math::Calculator m_math_calc;
	};
}
