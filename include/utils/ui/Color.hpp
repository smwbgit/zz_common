#pragma once

namespace utils::ui
{
	//颜色
	class Color
	{
	public:
		double r;
		double g;
		double b;
	public:
		//白色
		static Color white()
		{
			return {255,255,255};
		}
	};
}