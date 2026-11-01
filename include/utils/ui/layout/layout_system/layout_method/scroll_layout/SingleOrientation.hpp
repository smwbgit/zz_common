#pragma once

#include <utils/ui/layout/layout_system/common.hpp>
#include <utils/math/overlap/full_overlap.hpp>
#include <concepts>

namespace utils::ui::layout::layout_system::layout_method::scroll_layout
{
	//向前声明
	class ScrollLayout;
}

//单方向的滚动
namespace utils::ui::layout::layout_system::layout_method::scroll_layout::single_orientation
{
	//基础版
	//note: 无限制，可以把子窗口放置到视口之外
	class Unlimited
	{
	public:
		//是否开启。当不开启时，此方向无滚动功能
		utils::value::property::Property<bool> m_is_enable;
		//滚动位置，也就是子窗口的位置
		utils::value::property::Property<s8> m_scroll_position;
	protected:
		utils::value::property::Property<s8> m_window_length;	//这个版本，权宜之计。详情参见 Limited::Limited 里的说明
		utils::value::property::Property<std::optional<s8>> m_child_window_length;	//当没有子窗口时，为空

#pragma region 此类的核心函数
	public:
		Unlimited()
		{
			m_scroll_position.add_checker([this](s8 & new_value)
			{
				//获得是否开启
				const auto is_enable = m_is_enable.get_value();

				//未开启
				if(!is_enable)
				{
					//滚动位置，固定在0
					new_value = 0;

					//返回
					return;
				}
			});
		}
	protected:
		friend class ScrollLayout;

		//计算需求
		SingleOrientationRequirement calculate_requirement(
			const SingleOrientationRequirement & requirement) const
		{
			//获取是否开启滚动
			const auto is_enable = m_is_enable.get_value();

			//没有开启滚动
			if(!is_enable)
			{
				//直接使用子窗口的需求
				return requirement;
			}

			//准备数据
			SingleOrientationRequirement _requirement;

			//设置数据
			_requirement.min_size = 0;
			_requirement.hint_size = requirement.hint_size;
			_requirement.max_size = LayoutUnlimitedValue;

			//给出
			return _requirement;
		}
		//计算布局
		utils::Range1D calculate_layout(const s8 window_size,
			const SingleOrientationRequirement & child_requirement)
		{
			//获取是否开启滚动
			const auto is_enable = m_is_enable.get_value();

			//没有开启滚动
			if(!is_enable)
			{
				//偏移为0，并使用所有大小
				utils::Range1D rect;
				rect.position = 0;
				rect.size = window_size;

				//给出
				return rect;
			}

			//准备数据
			utils::Range1D rect;

			//先计算子窗口的大小

			//子窗口大小，取 最优大小 和 当前窗口 两者的较大值（同时不超过子窗口的最大值），以实现，
			//    缩小时，以最优大小呈现，变大时，又尽量不留白

			//子窗口大小
			rect.size = std::clamp(window_size,
				child_requirement.hint_size, child_requirement.max_size);

			//更新缓存大小
			//note: 因为是使用的缓存，所以，对设置代码的位置有要求。要在获取滚动位置之前，以
			//      避免随后的窗口放置时，窗口位置的旧值覆盖新值。
			m_window_length.set_value(window_size);
			m_child_window_length.set_value(rect.size);

			//滚动位置
			rect.position = m_scroll_position.get_value();

			//给出
			return rect;
		}
#pragma endregion
	};

	namespace concepts
	{
		//概念: 滚动条
		template<typename T>
		concept ScrollBar = requires(T a){
			{a.m_scroll_position} -> std::same_as<utils::value::property::Property<s8> &>;
			{a.m_scroll_range} -> std::same_as<utils::value::property::ReadOnlyProperty<utils::Range1D> &>;
			{a.m_page_step} -> std::same_as<utils::value::property::ReadOnlyProperty<s8> &>;
			{a.m_single_step} -> std::same_as<utils::value::property::Property<std::optional<s8>> &>;
			{a._m_is_bind} -> std::same_as<bool &>;
		};
	}

	//增强版
	//note: 有限制，子窗口只能在视口内
	class Limited : public Unlimited
	{
	public:
		//滚动范围
		//note: 其中的每个值，都是 m_scroll_pos 的可允许取值
		//note: 当不可滚动时，范围的大小为1
		//note: 范围的大小最小为1
		utils::value::property::ReadOnlyProperty<utils::Range1D> m_scroll_range{{0,1}};
		//页步长
		utils::value::property::ReadOnlyProperty<s8> m_page_step{1};
		//单步长
		//note: 当未被用户设置时，为空。此时使用默认值
		utils::value::property::Property<std::optional<s8>> m_single_step;
	private:
		//写入器
		utils::value::property::ReadOnlyPropertyController<utils::Range1D> m_scroll_range_controller{m_scroll_range};
		//写入器
		utils::value::property::ReadOnlyPropertyController<s8> m_page_step_controller{m_page_step};

#pragma region 此类的核心函数
	public:
		Limited()
		{
			m_scroll_range_controller.add_checker([this](utils::Range1D & new_value)
			{
				//简单检查
				utils_assert(new_value.size >= 1);
				utils_assert(new_value.get_last_value() == 0);
			});
			m_scroll_range += [this]()
			{
				//滚动范围 改变后，检查一下 滚动位置
				m_scroll_position.check_value();
			};
			m_scroll_position.add_checker([this](s8 & new_value)
			{
				//获得当前的滚动范围
				const auto scroll_range = m_scroll_range.get_value();
				
				//简单检查
				utils_assert(scroll_range.size >= 1);

				//把滚动位置，限制在滚动范围内
				scroll_range.clamp_inplace(new_value);
			});

			//监听本窗口和子窗口的大小，当改变时，重新计算滚动范围（和页步长）
			//因为目前还没有子窗口添加事件，所以，这一版，先在这个类里，缓存双窗口的大小。

			//本窗口 长度改变 事件处理
			m_window_length += [this]()
			{
				//更新滚动范围
				update_scroll_range();

				//获取窗口长度
				const auto window_length = m_window_length.get_value();

				//简单检查
				utils_assert(window_length >= 0);

				//新的页步长
				//note: 为了满足全局的设计，当子窗口长度为零时，页步长规定为1。
				const auto page_step = window_length > 0 ? window_length : 1;

				//设置页步长
				m_page_step_controller.set_value(page_step);
			};
			//子窗口 长度改变 事件处理
			m_child_window_length += [this]()
			{
				//更新滚动范围
				update_scroll_range();
			};
		}
		//和指定滚动条捆绑
		void add_scroll_bar(concepts::ScrollBar auto & scroll_bar)
		{
			//简单检查
			utils_assert(!scroll_bar._m_is_bind);

			//已被绑定
			scroll_bar._m_is_bind = true;

			//绑定相应值
			m_scroll_position.add_bind(scroll_bar.m_scroll_position);
			m_scroll_range   .add_bind(scroll_bar.m_scroll_range   );
			m_page_step      .add_bind(scroll_bar.m_page_step      );
			m_single_step    .add_bind(scroll_bar.m_single_step    );

			//这个版本，先不提供解绑操作

			//如果此方向还未开启滚动，开启一下
			m_is_enable.set_value(true);
		}
	private:
		//更新滚动范围
		void update_scroll_range()
		{
			//准备函数
			auto no_scroll = [this]()
			{
				m_scroll_range_controller.set_value({0,1});
			};

			//获得是否开启
			const auto is_enable = m_is_enable.get_value();

			//未开启
			if(!is_enable)
			{
				//不可滚动
				return no_scroll();
			}
			
			//获取窗口长度
			const auto window_length = m_window_length.get_value();
			//获取子窗口长度
			const auto child_window_length = m_child_window_length.get_value();

			//没有子窗口
			if(!child_window_length.has_value())
			{
				//不可滚动
				return no_scroll();
			}

			//子窗口长度
			const auto _child_window_length = child_window_length.value();

			//简单检查
			utils_assert(window_length >= 0);
			utils_assert(_child_window_length >= 0);

			//窗口/子窗口的长度为0
			if( window_length == 0 || _child_window_length == 0 )
			{
				//不可滚动
				return no_scroll();
			}

			//获得允许的范围
			const auto allow_range = utils::math::overlap::full_overlap::get_allow_range(
				window_length, _child_window_length);

			//如果当前窗口大，子窗口小，或他们相等，则不可滚动
			//如果当前窗口小，子窗口大，则滚动范围，为 [allow_range , 0]

			//当前窗口较大（或相等）
			if(allow_range >= 0)
			{
				//不可滚动
				return no_scroll();
			}
			//当前窗口较小
			else
			{
				//滚动范围，为 [allow_range , 0]

				//起始位置
				const auto position = allow_range;

				//范围大小
				const auto size = 1 - allow_range;

				//设置滚动范围
				m_scroll_range_controller.set_value({position, size});

				//返回
				return;
			}
		}
#pragma endregion
	private:
		friend class ScrollLayout;

		//当前无子窗口
		void now_no_child()
		{
			//无子窗口
			m_child_window_length.set_value({});
		}
	};
}