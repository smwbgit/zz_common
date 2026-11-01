#pragma once

namespace utils::ui::layout
{
	//对齐方式
	enum class AlignValue
	{
		Small,	//向小方向对齐, 比如左边，上边
		Middle,	//向中间对齐
		Large,	//向大方向对齐, 比如右边，下边
	};
	//对齐方式
	struct Align
	{
		AlignValue horizontal{};	//水平方向上
		AlignValue vertical{};		//垂直方向上

	public:
		auto operator<=>(const Align &) const = default;
	};
}