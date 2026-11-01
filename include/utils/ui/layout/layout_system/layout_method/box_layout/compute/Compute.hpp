#pragma once

#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/common.hpp>
#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/copy_params.hpp>
#include <utils/ui/layout/layout_system/layout_method/box_layout/compute/change_orientation.hpp>

namespace utils::ui::layout::layout_system::layout_method::box_layout::compute
{
	//线性布局的全部核心计算函数
	//note: 此类里，全是静态函数
	class Compute
	{
		//在这里引用一下，以便于外部用户，可以按照这类函数的一般写法，能在类内部，引用到函数参数的定义
	public:
		using calculate_requirement__params = calculate_requirement__params;
		using calculate_layout__params = calculate_layout__params;

#pragma region 此类的核心函数
	public:
		//计算需求
		static Requirement calculate_requirement(const calculate_requirement__params & params)
		{
			//按垂直布局和水平布局，分别调用单独的函数
			if(params.setting->is_vertical_layout)
			{
				//转发
				return calculate_requirement_vertical(params);
			}
			else
			{
				//转发
				return calculate_requirement_horizontal(params);
			}
		}
		//计算布局
		static auto calculate_layout(const calculate_layout__params & params)
			-> std::vector<utils::Range2D>
		{
			//按垂直布局和水平布局，分别调用单独的函数
			if(params.setting->is_vertical_layout)
			{
				//转发
				return calculate_layout_vertical(params);
			}
			else
			{
				//转发
				return calculate_layout_horizontal(params);
			}
		}
#pragma endregion

#pragma region 此类的辅助函数

#pragma region 计算布局
	private:
		//计算布局: 对于垂直布局
		static auto calculate_layout_vertical(const calculate_layout__params & params)
			-> std::vector<utils::Range2D>
		{
			//准备数据：把函数参数的每一项，变成一个引用，方便后面使用
			const auto & window_size = params.window_size;
			const auto & requirement = *params.requirement;
			const auto & all_requirement = *params.all_requirement;
			const auto is_vertical_layout = params.setting->is_vertical_layout;
			const auto & align = params.setting->align;
			const auto & inside_padding = params.setting->inside_padding;
			const auto & outside_padding = params.setting->outside_padding;

			//简单检查
			utils_assert(is_vertical_layout);
			utils_assert(!all_requirement.empty());

			//给每个窗口，分配它的hint_size
			std::vector<utils::Range2D> rects;
			for(const auto & one : all_requirement)
			{
				rects.push_back({{},one.hint_size});
			}

			//考虑垂直方向

			//检查是该拉伸还是该压缩
			if(window_size.j == requirement.hint_size.j)
			{
				//不需要伸缩
				(void)0;
			}
			else if(window_size.j > requirement.hint_size.j)
			{
				//需要拉伸

				//检查每个窗口，把它能拉伸的量，拉到最大，直到没有需要分配的像素

				//需要分配的像素数
				s8 rest_pixels = window_size.j - requirement.hint_size.j;

				for(const auto k : utils::iterator_index(rects))
				{
					//简单检查
					utils_assert(
						all_requirement[k].max_size.j >=
						all_requirement[k].hint_size.j);

					//窗口的最大拉伸量
					const s8 size_1 =
						all_requirement[k].max_size.j -
						all_requirement[k].hint_size.j;

					//计算最大的分配量
					const s8 size_2 = (std::min)(size_1, rest_pixels);

					//分配一些像素
					rects[k].size.j += size_2;
					rest_pixels -= size_2;
					
					//如果已全部分完，则停止
					if(rest_pixels == 0)
						break;
				}
			}
			else
			{
				//需要压缩

				//检查每个窗口，把它能压缩的量，压到最大，直到没有需要分配的像素

				//需要分配的像素数
				s8 rest_pixels = requirement.hint_size.j - window_size.j;

				for(const auto k : utils::iterator_index(rects))
				{
					//简单检查
					utils_assert(
						all_requirement[k].hint_size.j >=
						all_requirement[k].min_size.j);

					//窗口的最大压缩量
					const s8 size_1 =
						all_requirement[k].hint_size.j -
						all_requirement[k].min_size.j;

					//计算最大的分配量
					const s8 size_2 = (std::min)(size_1, rest_pixels);

					//分配一些像素
					rects[k].size.j -= size_2;
					rest_pixels -= size_2;
					
					//如果已全部分完，则停止
					if(rest_pixels == 0)
						break;
				}
			}

			//考虑水平方向

			//在去掉外边距后，父窗口的宽度
			//在去掉外边距后，如果父窗口已经没有像素了，则把父窗口的剩余宽度当做0处理
			const s8 child_width = [&]() -> s8
			{
				//外边距的宽度
				const auto padding = outside_padding.left + outside_padding.right;

				if (padding >= window_size.i)
					return 0;
				else
					return window_size.i - padding;
			}();

			//把窗口宽度设为“去掉外边距后的父窗口宽度”（在窗口的 [min_size, max_size] 限制内）
			for(const auto k : utils::iterator_index(rects))
			{
				//简单检查
				utils_assert(
					all_requirement[k].max_size.i >=
					all_requirement[k].hint_size.i);
				utils_assert(
					all_requirement[k].hint_size.i >=
					all_requirement[k].min_size.i);

				rects[k].size.i = std::clamp( child_width,
					all_requirement[k].min_size.i,
					all_requirement[k].max_size.i );
			}

			//现在子窗口的宽高都安排好了，接下来来安排它们的位置
			
			//考虑垂直方向: 位置设定

			//一个一个子窗口走过去，每走过一个，就把它的高度累加

			//当前位置
			s8 position_y{};

			//先是外边距，跳过它
			position_y += outside_padding.top;

			for(const auto k : utils::iterator_index(rects))
			{
				rects[k].position.j = position_y;

				//跳过这个子窗口的高度
				position_y += rects[k].size.j;

				//再跳过内边距，如果不是最后一个子窗口的话
				if (k+1 < rects.size())
				{
					position_y += inside_padding.value;
				}
			}

			//再次跳过外边距
			position_y += outside_padding.bottom;

			//如果我们还没有到窗口的边缘，或者已经超过，说明所有的窗口都已伸缩到最大程度，
			//但仍没有满足父窗口的高度。这时，不再调整子窗口的大小，也不调整它们的相对位置，
			//而是根据垂直方向的对齐设置，整体偏移所有子窗口。

			//需要做整体偏移
			if(position_y != window_size.j)
			{
				//算出所有子窗口的高度和，包括它们之间的内边距
				s8 total_height{};
				for(const auto k : utils::iterator_index(rects))
				{
					total_height += rects[k].size.j;
				}
				total_height += ((s8)rects.size() - 1) * inside_padding.value;

				//根据垂直方向的对齐设置：
				//如果是上对齐，则把他们的上边缘，对齐到父窗口上边的外边距上
				//如果是下对齐，则把他们的下边缘，对齐到父窗口下边的外边距上
				//如果是居中对齐，则把他们的中间线，对齐到父窗口的中间线上

				//先计算出 子窗口整体 的上边缘的所在位置
				//然后得出整体的要做的偏移量，再把这个偏移量，应用在每个子窗口上，到此完成整体偏移

				//子窗口们做为一个整体，它们上边缘的所在位置
				s8 items_y{};

				//上对齐
				if(align.vertical == AlignValue::Small)
				{
					items_y = outside_padding.top;
				}
				//下对齐
				else if(align.vertical == AlignValue::Large)
				{
					items_y =
						window_size.j - 1 - outside_padding.bottom - total_height;
				}
				//居中对齐
				else if(align.vertical == AlignValue::Middle)
				{
					items_y = outside_padding.top +
						(window_size.j - total_height - outside_padding.top - outside_padding.bottom) / 2;
				}
				//简单检查
				else
					utils_throw();

				//每个子窗口的偏移量 = 整体的目标位置 - 整体的目前位置
				const s8 offset = items_y - outside_padding.top;

				//给每个子窗口应用偏移量
				for(const auto k : utils::iterator_index(rects))
				{
					rects[k].position.j += offset;
				}

				//到此完成整体偏移
			}

			//整体偏移好后，垂直方向的位置设定 就已完成
			
			//考虑水平方向: 位置设定

			//根据水平方向的对齐设置：
			//当为左对齐时，把窗口的左边缘放在左边的外边距上
			//当为右对齐时，把窗口的右边缘放在右边的外边距上
			//当为居中对齐时，把 窗口的中间线 放在 父窗口的中间线 上

			//左对齐
			if(align.horizontal == AlignValue::Small)
			{
				for(const auto k : utils::iterator_index(rects))
				{
					rects[k].position.i = outside_padding.left;
				}
			}
			//右对齐
			else if(align.horizontal == AlignValue::Large)
			{
				for(const auto k : utils::iterator_index(rects))
				{
					rects[k].position.i =
						window_size.i - 1 - outside_padding.right - rects[k].size.i;
				}
			}
			//居中对齐
			else if(align.horizontal == AlignValue::Middle)
			{
				for(const auto k : utils::iterator_index(rects))
				{
					rects[k].position.i = outside_padding.left +
						(window_size.i - rects[k].size.i - outside_padding.left - outside_padding.right) / 2;
				}
			}
			//简单检查
			else
				utils_throw();

			//至此，所有子窗口的位置和大小，都设置好了

			//给出
			return rects;
		}
		//计算布局: 对于水平布局
		static auto calculate_layout_horizontal(const calculate_layout__params & params)
			-> std::vector<utils::Range2D>
		{
			//简单检查
			utils_assert(!params.setting->is_vertical_layout);

			//重用垂直布局的代码，以避免再写一份很相同的代码

			//为了重用垂直布局的代码，将会横竖变换一些参数，来模拟垂直布局。在计算完后，
			//    再把计算结果变换回来

			//复制参数
			auto packed_params = copy_params::get_packed_params(params);

			//新的参数
			auto & _params = packed_params.params;

			//横竖变换
			change_orientation::change_inplace(_params);

			//调用 垂直布局 来计算
			auto result = calculate_layout_vertical(_params);

			//对结果进行横竖变换
			change_orientation::change_inplace(result);

			//至此，计算完成

			//给出
			return result;
		}
#pragma endregion
#pragma region 计算需求
	private:
		//计算需求: 垂直布局
		static Requirement calculate_requirement_vertical(const calculate_requirement__params & params)
		{
			//简单检查
			utils_assert(params.setting->is_vertical_layout);
			utils_assert(!params.all_requirement->empty());

			//对于垂直方向，相加，对于水平方向，取最大值

			//准备数据
			Requirement requirement;

			//对于每个子窗口
			for(const auto & child_requirement : *params.all_requirement)
			{
				//准备数据
				auto & a = requirement;
				auto & b = child_requirement;

				//相加
				a.min_size .j += b.min_size .j;
				a.hint_size.j += b.hint_size.j;
				a.max_size .j += b.max_size .j;

				//取最大值
				a.min_size .i = (std::max)( a.min_size .i, b.min_size .i );
				a.hint_size.i = (std::max)( a.hint_size.i, b.hint_size.i );
				a.max_size .i = (std::max)( a.max_size .i, b.max_size .i );
			}

			//再加上内边距和外边距

			//子窗口数量
			const auto child_n = (s8)params.all_requirement->size();

			//垂直方向的总边距
			const auto vertical_total_padding =
				params.setting->inside_padding.value * (child_n - 1) +
				params.setting->outside_padding.top +
				params.setting->outside_padding.bottom;

			//补上它
			requirement.min_size .j += vertical_total_padding;
			requirement.hint_size.j += vertical_total_padding;
			requirement.max_size .j += vertical_total_padding;

			//水平方向的总边距
			const auto horizontal_total_padding =
				params.setting->outside_padding.left +
				params.setting->outside_padding.right;

			//也补上它
			requirement.min_size .i += horizontal_total_padding;
			requirement.hint_size.i += horizontal_total_padding;
			requirement.max_size .i += horizontal_total_padding;

			//至此，计算完成

			//给出
			return requirement;
		}
		//合并需求: 水平布局
		static Requirement calculate_requirement_horizontal(const calculate_requirement__params & params)
		{
			//简单检查
			utils_assert(!params.setting->is_vertical_layout);
			
			//复制参数
			auto packed_params = copy_params::get_packed_params(params);

			//新的参数
			auto & _params = packed_params.params;

			//横竖变换
			change_orientation::change_inplace(_params);

			//调用 垂直布局 来计算
			auto result = calculate_requirement_vertical(_params);

			//对结果进行横竖变换
			change_orientation::change_inplace(result);

			//给出
			return result;
		}
#pragma endregion
#pragma endregion

	};
}