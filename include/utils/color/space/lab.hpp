#pragma once

#include <utils/color/space/xyz.hpp>
#include <utils/integer_type.hpp>

//颜色空间: lab*
namespace utils::color::space::lab
{
	//一种 lab 颜色
	class Color
	{
	public:
		double l{};
		double a{};
		double b{};
	};

	//一些常量
	namespace inner
	{
		//note: ai生成，稍作修改

		inline constexpr double Xn = 0.95047;   // D65 白点
		inline constexpr double Yn = 1.00000;
		inline constexpr double Zn = 1.08883;
		inline constexpr double delta = 6.0 / 29.0;   // CIE 阈值 (6/29)^3≈0.008856
		inline constexpr double delta2 = delta * delta;
		inline constexpr double delta3 = delta2 * delta;
		inline constexpr double kappa = 903.3;         // 1.0/0.008856*7.787+4/29 近似
	}

	//xyz转lab
	inline Color from_xyz(const utils::color::space::xyz::Color xyz)
	{
		//note: ai生成，稍作修改

		//准备数据
		double X = xyz.x;
		double Y = xyz.y;
		double Z = xyz.z;

		//导入常量
		using namespace inner;

		//简单检查: 负值无物理意义
		utils_assert(X >= 0.0 && Y >= 0.0 && Z >= 0.0);

		double fx = (X > delta3) ? std::cbrt(X / Xn) : (kappa * (X / Xn) + 16.0) / 116.0;
		double fy = (Y > delta3) ? std::cbrt(Y / Yn) : (kappa * (Y / Yn) + 16.0) / 116.0;
		double fz = (Z > delta3) ? std::cbrt(Z / Zn) : (kappa * (Z / Zn) + 16.0) / 116.0;

		double L = 116.0 * fy - 16.0;
		double a = 500.0 * (fx - fy);
		double b = 200.0 * (fy - fz);

		//简单检查: L* 法定范围 0-100，a*b* 理论无界
		utils_assert(L >= 0.0 && L <= 100.0);

		//给出
		return { L, a, b };
	}
	//lab转xyz
	inline utils::color::space::xyz::Color to_xyz(const Color lab)
	{
		//note: ai生成，稍作修改

		//准备数据
		double L = lab.l;
		double a = lab.a;
		double b = lab.b;
		
		//导入常量
		using namespace inner;

		//简单检查: 超出即逻辑错误
		utils_assert(L >= 0.0 && L <= 100.0);

		double fy = (L + 16.0) / 116.0;
		double fx = fy + a / 500.0;
		double fz = fy - b / 200.0;

		//准备函数: f 逆函数
		auto f_inv = [](double t) -> double
		{
			//简单检查: L*a*b* 定义域要求
			utils_assert(t >= 4.0 / 29.0);

			return (t > delta) ? t * t * t
							   : (t - 4.0 / 29.0) * kappa / 116.0;
		};

		double Xr = f_inv(fx);
		double Yr = f_inv(fy);
		double Zr = f_inv(fz);

		double X = Xr * Xn;
		double Y = Yr * Yn;
		double Z = Zr * Zn;

		//简单检查: 结果必须非负
		utils_assert(X >= 0.0 && Y >= 0.0 && Z >= 0.0);

		//给出
		return { X, Y, Z };
	}

	//便利函数: rgb转lab
	inline Color from_rgb(const utils::color::Color rgb)
	{
		//获取xyz
		const auto xyz = utils::color::space::xyz::from_rgb(rgb);

		//获取lab
		const auto lab = from_xyz(xyz);

		//给出
		return lab;
	}
	//便利函数: lab转rgb
	inline utils::color::Color to_rgb(const Color lab)
	{
		//获取xyz
		const auto xyz = to_xyz(lab);

		//获取rgb
		const auto rgb = utils::color::space::xyz::to_rgb(xyz);

		//给出
		return rgb;
	}

	//获取两种颜色的距离
	inline double get_color_distance(const Color color_1, const Color color_2)
	{
		//note: ai生成，稍作修改

		//准备数据
		const auto L1 = color_1.l;
		const auto a1 = color_1.a;
		const auto b1 = color_1.b;
		const auto L2 = color_2.l;
		const auto a2 = color_2.a;
		const auto b2 = color_2.b;

		//简单检查: Lab* 法定范围：L 0-100，a、b 无界但负值合法，这里只拦 NaN/Inf
		utils_assert(std::isfinite(L1) && std::isfinite(a1) && std::isfinite(b1));
		utils_assert(std::isfinite(L2) && std::isfinite(a2) && std::isfinite(b2));

		double dL = L1 - L2;
		double da = a1 - a2;
		double db = b1 - b2;

		return std::sqrt(dL * dL + da * da + db * db);
	}
}