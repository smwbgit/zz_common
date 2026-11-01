#pragma once

#include <utils/color/Color.hpp>
#include <utils/assert.hpp>

//颜色空间
namespace utils::color::space
{
}

//颜色空间: xyz
namespace utils::color::space::xyz
{
	//一种 xyz 颜色
	class Color
	{
	public:
		double x{};
		double y{};
		double z{};
	};

	//rgb转xyz
	inline Color from_rgb(const utils::color::Color rgb)
	{
		//note: ai生成，稍作修改

		//归一化 [0,1]
		double r = rgb.r / 255.0;
		double g = rgb.g / 255.0;
		double b = rgb.b / 255.0;

		//逆 gamma：输入必须 0-1，否则 pow 行为未定义
		auto invG = [](double c)
		{
			//简单检查
			utils_assert(c >= 0.0 && c <= 1.0);

			return (c <= 0.04045) ? (c / 12.92)
								   : std::pow((c + 0.055) / 1.055, 2.4);
		};
		double rL = invG(r);
		double gL = invG(g);
		double bL = invG(b);

		//标准 sRGB D65 矩阵
		double X = 0.4124564 * rL + 0.3575761 * gL + 0.1804375 * bL;
		double Y = 0.2126729 * rL + 0.7151522 * gL + 0.0721750 * bL;
		double Z = 0.0193339 * rL + 0.1191920 * gL + 0.9503041 * bL;

		//理论上 XYZ 必须 ≥0；出现负值说明矩阵/数据出错
		utils_assert(X >= 0.0 && Y >= 0.0 && Z >= 0.0);

		return { X, Y, Z };
	}
	//xyz转rgb
	inline utils::color::Color to_rgb(const Color xyz)
	{
		//note: ai生成，稍作修改

		//准备数据
		double X = xyz.x;
		double Y = xyz.y;
		double Z = xyz.z;

		//简单检查: 负输入无物理意义
		utils_assert(X >= 0.0 && Y >= 0.0 && Z >= 0.0);

		//反矩阵：XYZ -> 线性 RGB
		double rL =  3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z;
		double gL = -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z;
		double bL =  0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z;

		//准备函数: 把 double 钳到 [0,1]
		auto clamp01 = [](double v)
		{
			return (v < 0.0) ? 0.0 : (v > 1.0) ? 1.0 : v;
		};

		//线性 RGB 可能超色域，必须先钳位再 gamma
		rL = clamp01(rL);
		gL = clamp01(gL);
		bL = clamp01(bL);

		//gamma 编码：输入必须 0-1
		auto gamma = [](double c)
		{
			//简单检查
			utils_assert(c >= 0.0 && c <= 1.0);

			return (c <= 0.0031308) ? 12.92 * c
									 : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
		};
		double r = gamma(rL);
		double g = gamma(gL);
		double b = gamma(bL);

		//最后转 0-255 并四舍五入
		auto toU8 = [clamp01](double f)
		{
			f = clamp01(f) * 255.0 + 0.5;
			return static_cast<uint8_t>(f);
		};
		return { toU8(r), toU8(g), toU8(b) };
	}
}