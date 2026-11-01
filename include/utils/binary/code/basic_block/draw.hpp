#pragma once

#include <windows.h>
#include <windowsx.h>
#include <utils/binary/code/basic_block.hpp>
#include <utils/bidirectional_map.hpp>
#include <utils/windows/ui/window/timer.hpp>
#include <utils/windows/ui/window/create.hpp>
#include <utils/windows/ui/window/size.hpp>
#include <utils/windows/ui/window/paint.hpp>
#include <utils/windows/ui/window/paint/gdi.hpp>
#include <utils/memory.hpp>
#include <utils/math/s_maxmin_value.hpp>
#include <utils/binary/code/basic_block/VariableManager.hpp>

//车道分配
namespace utils::binary::code::basic_block::draw::lane
{
	namespace inner
	{
		//进出口的车道分配
		class EntryLaneAllocator
		{
		private:
			const uint64_t m_max_lane = 10;	//返回的车道会小于等于这个值
			std::map<utils::Index2D, uint64_t> m_lanes;
		public:
			uint64_t alloc(utils::Index2D pos)
			{
				const auto lane = m_lanes[pos];

				if(lane < m_max_lane)
					m_lanes[pos]++;

				return lane;
			}
		};
		//横线的车道分配
		class HorizontalLaneAllocator
		{
		private:
			const uint64_t m_max_lane = 5;	//返回的车道会小于等于这个值
			std::map<
				int64_t /*y*/, std::map<
				uint64_t /*lane*/, std::vector<
				utils::Range1D<int64_t> /*每个已用的位置*/>>> m_lanes;	//todo 看看还有没有更简洁/更好算的表示方法
		public:
			uint64_t alloc(utils::Index2D from, utils::Index2D to)
			{
				//简单检查
				utils_assert(from.j == to.j);
				utils_assert(from.i != to.i);

				//想分配的区域
				utils::Range1D<int64_t> want_range;
				{
					if(from.i < to.i)
						want_range.set(from.i, to.i - from.i + 1);
					else
						want_range.set(to.i, from.i - to.i + 1);
				}

				//遍历每个车道，看看是否能分配
				for(uint64_t lane = 0; lane < m_max_lane; lane++)
				{
					//查找车道现状
					auto & allocated = m_lanes[from.j][lane];

					//遍历每个已分配的区域，看看是否能放下
					bool is_ok = true;
					for(const auto & one : allocated)
					{
						if(one.is_overlap(want_range))
						{
							is_ok = false;
							break;
						}
					}

					//找到一个可以分配的车道
					if (is_ok)
					{
						//添加记录
						allocated.push_back(want_range);

						return lane;
					}
				}

				//返回最大车道
				return m_max_lane;
			}
		};
		//竖线的车道分配
		class VerticalLaneAllocator
		{
		private:
			HorizontalLaneAllocator m_allocator;
		public:
			uint64_t alloc(utils::Index2D from, utils::Index2D to)
			{
				//转发
				return m_allocator.alloc({from.j,from.i},{to.j,to.i});
			}
		};
	}
	//给所有的线分配车道
	//传入: from和to
	//传出: 它的5个车道
	class LineManager
	{
	public:
		/*
			每条连线会有5根线，分别是：
				起始
				横向
				纵向
				第二次横向
				结束
			特殊情况会有1根(在正下方)和3根(目标在下一排), 此时m_single_lines[0-4].m_is_exist表示线是否存在
		*/
		struct s_single_line
		{
			bool m_is_exist{true};
			uint64_t m_lane{};
		};
		struct s_line
		{
			utils::Index2D m_from;
			utils::Index2D m_to;
			s_single_line m_single_lines[5];
		};
	public:
		std::vector<s_line> m_all_line;
	private:
		inner::EntryLaneAllocator m_entry_lane_allocator;
		inner::HorizontalLaneAllocator m_horizontal_lane_allocator;
		inner::VerticalLaneAllocator m_vertical_lane_allocator;
	public:
		void add_line(utils::Index2D from, utils::Index2D to)
		{
			//保存原始信息
			s_line line;
			line.m_from = from;
			line.m_to = to;

			//为每个逻辑线路分配车道
			if (from.j + 1 == to.j)	//当后继结点在下一排时，特殊处理
			{
				line.m_single_lines[2].m_is_exist = false;
				line.m_single_lines[3].m_is_exist = false;

				if (from.i == to.i)	//当后继结点在正下方时，特殊处理
				{
					line.m_single_lines[1].m_is_exist = false;
					line.m_single_lines[4].m_is_exist = false;
				}
				else
				{
					line.m_single_lines[1].m_lane = alloc_lane_for_horizontal_line(
						{get_intricate_pos(from.i, true), from.j+1},
						{get_intricate_pos(to.i, true), to.j});
					line.m_single_lines[4].m_lane = alloc_lane_for_entry(to);
				}
			}
			else
			{
				line.m_single_lines[1].m_lane = alloc_lane_for_horizontal_line(
					{get_intricate_pos(from.i, true), from.j+1},
					{get_intricate_pos(to.i, false), from.j+1});
				line.m_single_lines[2].m_lane = alloc_lane_for_vertical_line(
					{to.i, get_intricate_pos(from.j+1, false)},
					{to.i, get_intricate_pos(to.j, false)});
				line.m_single_lines[3].m_lane = alloc_lane_for_horizontal_line(
					{get_intricate_pos(to.i, false), to.j},
					{get_intricate_pos(to.i, true), to.j});
				line.m_single_lines[4].m_lane = alloc_lane_for_entry(to);
			}
			line.m_single_lines[0].m_lane = alloc_lane_for_entry({from.i, from.j+1});

			//保存
			m_all_line.push_back(line);
		}
	private:
		uint64_t alloc_lane_for_entry(utils::Index2D pos)
		{
			return m_entry_lane_allocator.alloc(pos);
		}
		uint64_t alloc_lane_for_horizontal_line(utils::Index2D from, utils::Index2D to)
		{
			return m_horizontal_lane_allocator.alloc(from, to);
		}
		uint64_t alloc_lane_for_vertical_line(utils::Index2D from, utils::Index2D to)
		{
			return m_vertical_lane_allocator.alloc(from, to);
		}
		//获得精细位置
		int64_t get_intricate_pos(int64_t pos, bool is_right_above)
		{
			return pos*2 +(is_right_above?1:0);
		}
	};
}

//绘制图形
namespace utils::binary::code::basic_block::draw
{
	namespace inner
	{
		class BasicBlockWindow
		{
		private:
			const basic_block::Graph* m_graph{};
		private:
			utils::bidirectional_map<uint64_t, utils::Index2D> m_layout{};
			utils::Index2D m_layout_size{};
			basic_block::draw::lane::LineManager m_line_manager{};
			utils::Index2D m_ui_total_size;
			std::unique_ptr<utils::windows::ui::window::timer::TimerManager> m_timer_manager;
			std::unique_ptr<utils::binary::code::basic_block::VariableManager> m_variable_manager;
			//std::unique_ptr<utils::binary::code::basic_block::LoopManager> m_loop_manager;
			uint64_t m_ui_color_i{};
			bool m_is_inited{};
			std::optional<uint64_t> m_user_selected_block;

			inline static const utils::Index2D m_ui_basic_block_size{22, 13};
			inline static const utils::Index2D m_ui_scroll_size_for_line_step{20, 20};
			inline static const utils::Index2D m_ui_size_for_no_lane{2, 2};
			inline static const utils::Index2D m_ui_whitespace_size{20, 20};
		public:
			void set_args(const basic_block::Graph* graph)
			{
				m_graph = graph;
			}
			LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				switch (uMsg)
				{
				case WM_CREATE:
					return 0;
				case utils::windows::ui::window::create::user_message::UM_CREATE:
				{
					//生成布局和布线
					make_layer();

					//准备数据
					m_ui_total_size.i = m_layout_size.i * (m_ui_basic_block_size.i + m_ui_whitespace_size.i) + m_ui_whitespace_size.i;
					m_ui_total_size.j = m_layout_size.j * (m_ui_basic_block_size.j + m_ui_whitespace_size.j) + m_ui_whitespace_size.j;

					//简单检查
					utils_assert(m_ui_total_size.i > 0);
					utils_assert(m_ui_total_size.j > 0);
					utils_assert(m_ui_total_size.i < 65535);
					utils_assert(m_ui_total_size.j < 65535);
			
					//简单检查
					utils_assert(m_ui_basic_block_size.i > 0);
					utils_assert(m_ui_basic_block_size.j > 0);
					utils_assert(m_ui_whitespace_size.i > 0);
					utils_assert(m_ui_whitespace_size.j > 0);

					//计算更多信息
					std::thread([this](){
						utils::make_unique(m_variable_manager, *m_graph);
					//	utils::make_unique(m_loop_manager, m_entry, *m_blocks, m_block_details);
					}).detach();

					//启动定时器
					utils::make_unique(m_timer_manager, hwnd);
					m_timer_manager->add_timer(1000, [this]()
					{
						m_ui_color_i++;
					});

					//更新状态
					m_is_inited = true;

					//设置滚动条
					set_scroll_bar(hwnd);

					//添加菜单栏
					add_menu_bar(hwnd);

					utils::windows::ui::window::size::最大化(hwnd);
					return 0;
				}
				case WM_SIZE:
				{
					InvalidateRect(hwnd, 0, FALSE);

					//还未完成数据的传递
					if(!m_is_inited) return 0;

					//设置滚动条
					set_scroll_bar(hwnd);

					return 0;
				}
				case WM_TIMER:
				{
					m_timer_manager->meet_timer_message(wParam, lParam);

					//重绘窗口
					InvalidateRect(hwnd, NULL, FALSE);
					break;
				}
				case WM_PAINT:
				{
					utils::windows::ui::window::paint::begin_paint_with_memdc(hwnd, [this, hwnd](HDC memdc)
					{
						auto client_size = utils::windows::ui::window::size::get_client_size(hwnd);
						if (client_size.i <= 0 || client_size.j <= 0) return;

						//填充背景
						utils::windows::ui::window::paint::gdi::draw::rectangle::fill(memdc,
							{{}, client_size}, utils::image::color::Black);
				
						//还未完成数据的传递
						if(!m_is_inited) return;

						//获得窗口当前的滚动位置
						utils::Index2D scroll_pos = get_scroll_pos(hwnd);

						//准备函数
						auto draw_basic_block = [memdc, scroll_pos, this](utils::Index2D logic_pos)
						{
							const auto pos = get_ui_position_for_basic_block(logic_pos);
							const auto size = m_ui_basic_block_size;
							const auto color = [&]()
							{
								const auto colors = get_ui_color_for_basic_block(logic_pos);
								utils_assert(!colors.empty());
								return colors[m_ui_color_i%colors.size()];
							}();
							utils::windows::ui::window::paint::gdi::draw::rectangle::inner_border(memdc, {-scroll_pos + pos, size}, 2, color);
						};

						//画基本块
						iterator_basic_block_in_layout([draw_basic_block](
							utils::Index2D logic_pos, uint64_t block_addr, bool is_stop_loop)
						{
							draw_basic_block(logic_pos);
						});
				
						//准备函数
						auto get_x_for_entry = [this](utils::Index2D pos, uint64_t lane)
						{
							//todo 看看要不要把这个检查放到窗口创建时，检查最大的lane值
							//简单检查
							utils_assert((int64_t)lane * 2 < m_ui_basic_block_size.i);

							return pos.i * (m_ui_basic_block_size.i + m_ui_whitespace_size.i) + m_ui_whitespace_size.i + lane * 2;
						};
						auto get_y_for_exit = [this](utils::Index2D pos)
						{
							return pos.j * (m_ui_basic_block_size.j + m_ui_whitespace_size.j) + m_ui_whitespace_size.j + m_ui_basic_block_size.j;
						};
						auto get_y_for_entry = [this](utils::Index2D pos)
						{
							return pos.j * (m_ui_basic_block_size.j + m_ui_whitespace_size.j) + m_ui_whitespace_size.j - 1;
						};
						auto get_y_for_horizontal_line = [this](utils::Index2D pos, uint64_t lane)
						{
							//todo 看看能不能把这个检查放到窗口创建时，只检查一次
							//简单检查
							utils_assert(m_ui_size_for_no_lane.j*2 + (int64_t)lane * 2 < m_ui_whitespace_size.j);

							return pos.j * (m_ui_basic_block_size.j + m_ui_whitespace_size.j) + m_ui_size_for_no_lane.j + lane * 2;
						};
						auto get_x_for_vertical_line = [this](utils::Index2D pos, uint64_t lane)
						{
							//todo 看看能不能把这个检查放到窗口创建时，只检查一次
							//简单检查
							utils_assert(m_ui_size_for_no_lane.i*2 + (int64_t)lane * 2 < m_ui_whitespace_size.i);

							return pos.i * (m_ui_basic_block_size.i + m_ui_whitespace_size.i) + m_ui_size_for_no_lane.i + lane * 2;
						};
						auto draw_line =
							[
								get_x_for_entry,
								get_y_for_exit,
								get_y_for_entry,
								get_y_for_horizontal_line,
								get_x_for_vertical_line,
								memdc,
								scroll_pos
							]
							(const basic_block::draw::lane::LineManager::s_line & line)
						{
							namespace gdi = utils::windows::ui::window::paint::gdi;

							//简单检查
							utils_assert(line.m_single_lines[0].m_is_exist);

							if (line.m_from.j + 1 == line.m_to.j)	//当后继结点在下一排时，特殊处理
							{
								//简单检查
								utils_assert(!line.m_single_lines[2].m_is_exist);
								utils_assert(!line.m_single_lines[3].m_is_exist);

								if (line.m_from.i == line.m_to.i)	//当后继结点在正下方时，特殊处理
								{
									//简单检查
									utils_assert(!line.m_single_lines[1].m_is_exist);
									utils_assert(!line.m_single_lines[4].m_is_exist);
							
									const auto x = get_x_for_entry(
										{line.m_from.i, line.m_from.j+1},
										line.m_single_lines[0].m_lane);
									const auto y1 = get_y_for_exit(line.m_from);
									const auto y2 = get_y_for_entry(line.m_to);
									gdi::draw::line::straight_line(memdc,
										-scroll_pos + utils::Index2D{x,y1},
										-scroll_pos + utils::Index2D{x,y2+1},
										1, utils::image::color::Red);
								}
								else
								{
									//简单检查
									utils_assert(line.m_single_lines[1].m_is_exist);
									utils_assert(line.m_single_lines[4].m_is_exist);
							
									const auto x1 = get_x_for_entry(
										{line.m_from.i, line.m_from.j+1},
										line.m_single_lines[0].m_lane);
									const auto x2 = get_x_for_entry(
										line.m_to,
										line.m_single_lines[4].m_lane);
									const auto y1 = get_y_for_exit(line.m_from);
									const auto y2 = get_y_for_horizontal_line(
										line.m_to,
										line.m_single_lines[1].m_lane);
									const auto y3 = get_y_for_entry(line.m_to);
									gdi::draw::line::broken_line(memdc,
										{
											-scroll_pos + utils::Index2D{x1,y1},
											-scroll_pos + utils::Index2D{x1,y2},
											-scroll_pos + utils::Index2D{x2,y2},
											-scroll_pos + utils::Index2D{x2,y3+1}
										},
										1, utils::image::color::Red);
								}
							}
							else
							{
								//简单检查
								utils_assert(line.m_single_lines[1].m_is_exist);
								utils_assert(line.m_single_lines[2].m_is_exist);
								utils_assert(line.m_single_lines[3].m_is_exist);
								utils_assert(line.m_single_lines[4].m_is_exist);

								const auto x1 = get_x_for_entry(
									{line.m_from.i, line.m_from.j+1},
									line.m_single_lines[0].m_lane);
								const auto x2 = get_x_for_vertical_line(
									line.m_to,
									line.m_single_lines[2].m_lane);
								const auto x3 = get_x_for_entry(
									line.m_to,
									line.m_single_lines[4].m_lane);
								const auto y1 = get_y_for_exit(line.m_from);
								const auto y2 = get_y_for_horizontal_line(
									{line.m_from.i, line.m_from.j+1},
									line.m_single_lines[1].m_lane);
								const auto y3 = get_y_for_horizontal_line(
									line.m_to,
									line.m_single_lines[3].m_lane);
								const auto y4 = get_y_for_entry(line.m_to);
								gdi::draw::line::broken_line(memdc,
									{
										-scroll_pos + utils::Index2D{x1,y1},
										-scroll_pos + utils::Index2D{x1,y2},
										-scroll_pos + utils::Index2D{x2,y2},
										-scroll_pos + utils::Index2D{x2,y3},
										-scroll_pos + utils::Index2D{x3,y3},
										-scroll_pos + utils::Index2D{x3,y4+1}
									},
									1, utils::image::color::Red);
							}
						};

						//画连线
						for(const auto & line : m_line_manager.m_all_line)
						{
							draw_line(line);
						}

						//画当前块的信息
						if (m_user_selected_block.has_value())
						{
							RECT rect{};
							rect.left = client_size.i - 120;
							rect.top = 25;

							std::u8string text = utils::text::format::format(u8"0x%llx", m_user_selected_block.value());
							std::u16string text_u16 = utils::text::encoding::unicode::utf8_to_utf16(text);
							utils_assert(DrawTextW(memdc, (const wchar_t*)text_u16.c_str(), -1, &rect, DT_NOCLIP | DT_LEFT | DT_TOP));
						}
					});
					return 0;
				}
				case WM_LBUTTONDOWN:
				{
					//获得鼠标位置
					utils::Index2D cursor_pos{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

					//获得当前滚动位置
					utils::Index2D scroll_pos = get_scroll_pos(hwnd);

					//检查鼠标是否在某个基本块上
					decltype(m_user_selected_block) selected_block;	//不在所有块时，清空值
					iterator_basic_block_in_layout([&](utils::Index2D logic_pos, uint64_t block_addr, bool is_stop_loop)
					{
						//基本块的位置
						utils::Range2D range{
							get_ui_position_for_basic_block(logic_pos),
							m_ui_basic_block_size};
				
						//不在块上
						if (!range.is_include(cursor_pos + scroll_pos))
							return;

						selected_block = block_addr;
						is_stop_loop = true;
					});

					//更新数据和界面
					if (selected_block != m_user_selected_block)
					{
						m_user_selected_block = selected_block;

						//重绘窗口
						InvalidateRect(hwnd, NULL, FALSE);
					}
					break;
				}
				case WM_HSCROLL:
				{
					handle_scroll_message(hwnd, SB_HORZ, wParam, m_ui_scroll_size_for_line_step.i);
					break;
				}
				case WM_VSCROLL:
				{
					handle_scroll_message(hwnd, SB_VERT, wParam, m_ui_scroll_size_for_line_step.j);
					break;
				}
				case WM_CLOSE:
					break;
				case WM_DESTROY:
					PostQuitMessage(0);		//退出当前线程
					break;
				case WM_NCDESTROY:
					break;
				}
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		private:
			//生成布局和布线
			void make_layer()
			{
				//把每个块，按碰到的顺序，依次放入棋盘的某个格子中
				{
					struct s_block
					{
						uint64_t addr{};
						uint64_t x{};
					};
					uint64_t layer{};
					std::vector<s_block> cur_layer;
					std::vector<s_block> next_layer;
					std::set<uint64_t> visited;

					//准备函数
					auto put = [&cur_layer, &next_layer, &visited, this](
						const s_block & block, uint64_t layer, uint64_t & x)
					{
						//如果当前位置靠前，变为块的理想位置
						if (x < block.x)
							x = block.x;

						//放入棋盘
						m_layout.insert_not_exist(block.addr, {x, layer});

						//查找basic_block
						const auto & basic_block = m_graph->look_up_basic_block(block.addr);

						//把没遇到过的后继结点放入下一层
						for(const auto next : basic_block.m_next)
						{
							if (utils::set(visited).is_exist(next))
								continue;

							next_layer.push_back({next,x});
				
							//更新状态
							utils::set(visited).insert_not_exist(next);
						}

						//更新状态
						x++;
					};

					//把入口放进去,准备开始循环
					const auto entry = m_graph->get_entry();
					cur_layer.push_back({entry,0});

					//手动更新状态
					utils::set(visited).insert_not_exist(entry);

					while(1)
					{
						//把当前层的每个块，放入棋盘中, 把没遇到过的后继结点放入下一层
						uint64_t x{};
						for(const auto & one : cur_layer)
						{
							put(one, layer, x);
						}

						//都已循环完
						if(next_layer.empty()) break;

						//下一层变当前层
						cur_layer = next_layer; next_layer.clear();
						layer++;
					}
				}

				//布局总大小
				{
					utils::math::s_maxmin_value<uint64_t> x;
					utils::math::s_maxmin_value<uint64_t> y;

					for(const auto & one : m_layout.get_forward())
					{
						x.meet_one_value(one.second.i);
						y.meet_one_value(one.second.j);
					}

					//简单检查
					utils_assert(x.get_min() == 0);
					utils_assert(y.get_min() == 0);

					m_layout_size.i = x.get_max()+1;
					m_layout_size.j = y.get_max()+1;
				}

				//生成所有连线
				{
					//遍历每个基本块
					for(const auto & block : m_graph->get_basic_blocks())
					{
						//块标识
						const auto block_addr = block.second.m_addr;

						//查找棋盘内位置
						const auto pos = utils::map(m_layout.get_forward()).get_existed_value(block_addr);

						//对所有后继结点，生成连线
						for(const auto next : block.second.m_next)
						{
							//查找后继结点的棋盘位置
							const auto next_pos = utils::map(m_layout.get_forward()).get_existed_value(next);

							m_line_manager.add_line(pos, next_pos);
						}
					}
				}
			}
			//添加菜单栏
			void add_menu_bar(HWND hwnd)
			{
		#if 0
				//创建菜单
				HMENU hFileMenu = CreatePopupMenu(); utils_assert(hFileMenu);
				utils_assert(AppendMenuW(hFileMenu, MF_STRING|MF_CHECKED, 333, L"显示直接前继节点(黄)"));
				utils_assert(AppendMenuW(hFileMenu, MF_STRING|MF_CHECKED, 333, L"显示直接后继节点(绿)"));
				utils_assert(AppendMenuW(hFileMenu, MF_STRING|MF_CHECKED, 333, L"显示所有前继节点(蓝)"));
				utils_assert(AppendMenuW(hFileMenu, MF_STRING|MF_CHECKED, 333, L"显示所有后继节点(紫)"));
				utils_assert(AppendMenuW(hFileMenu, MF_STRING|MF_CHECKED, 333, L"隐藏其他无关节点"));

				HMENU hMenu = CreateMenu(); utils_assert(hMenu);
				utils_assert(AppendMenuW(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)hFileMenu, L"图追踪"));

				//将菜单与窗口关联
				utils_assert(SetMenu(hwnd, NULL));	//销毁原来的菜单
				utils_assert(SetMenu(hwnd, hMenu));
		#endif
			}
			//逻辑块的绘画颜色
			std::vector<utils::image::Color> get_ui_color_for_basic_block(utils::Index2D logic_pos)
			{
		#if 0
				is_ok;
				is_fine;
				is_fine;
				is_valid;
				is_need_update;
				is_invalid;
				is_updated;
				is_newest;
				is_latest;
				is_up_to_date;
				is_most_recent;
		#endif
				//if(m_ui_color_for_basic_block)
				//{
				//}
				//
				//return utils::map(m_ui_color_for_basic_block).get_existed_value(logic_pos);

				//todo: 看看还有没有更好的办法，来找出他们的颜色

				//如果被用户选中, 或者和选中节点有关联，则变色
				std::vector<utils::image::Color> colors;
				if(m_user_selected_block.has_value())
				{
					//只对循环节点变色
					if(1)
					{
						//查找被选中的基本块
						const auto block_addr = m_user_selected_block.value();
						const auto & block = m_graph->look_up_basic_block(block_addr);
			
						//查找当前基本块
						const auto cur_block_addr = utils::map(m_layout.get_backward()).get_existed_value(logic_pos);
						const auto & cur_block = m_graph->look_up_basic_block(cur_block_addr);
			
						//选中节点为循环节点
						if (block->m_is_loop_node)
						{
							//当前节点为循环节点
							if (cur_block->m_is_loop_node)
							{
								//被用户选中
								if(cur_block_addr == block_addr)
									colors.push_back(utils::image::color::Yellow);

								//互为循环的循环节点
								if (utils::set(block->m_downstream).is_exist(cur_block_addr) &&
									utils::set(block->m_upstream).is_exist(cur_block_addr))
									colors.push_back(utils::image::color::Cyan);
							}
						}
						//否则，提示用户，选中节点非循环节点
						else
						{
							//被用户选中
							if(cur_block_addr == block_addr)
								colors.push_back(utils::image::color::Red);
						}
					}
					//正常变色
					else
					{
						//查找被选中的基本块
						const auto block_addr = m_user_selected_block.value();
						const auto & block = m_graph->look_up_basic_block(block_addr);
			
						//查找当前基本块
						const auto cur_block_addr = utils::map(m_layout.get_backward()).get_existed_value(logic_pos);
						const auto & cur_block = m_graph->look_up_basic_block(cur_block_addr);
			
						//被用户选中
						if(cur_block_addr == block_addr)
							colors.push_back(utils::image::color::Yellow);
			
						//直接后继节点
						if(utils::set(block->m_immediate_successor).is_exist(cur_block_addr))
							colors.push_back(utils::image::color::Purple);

						//直接前继节点
						if(utils::set(block->m_immediate_predecessor).is_exist(cur_block_addr))
							colors.push_back(utils::image::color::Green);

						//下游节点
						if(utils::set(block->m_downstream).is_exist(cur_block_addr))
							colors.push_back(utils::image::color::Blue);

						//上游节点
						if(utils::set(block->m_upstream).is_exist(cur_block_addr))
							colors.push_back(utils::image::color::Cyan);
					}
				}
				//否则，让循环节点变色
				else
				{
					//查找当前基本块
					const auto cur_block_addr = utils::map(m_layout.get_backward()).get_existed_value(logic_pos);
					const auto & cur_block = m_graph->look_up_basic_block(cur_block_addr);

					//循环节点
					if (cur_block->m_is_loop_node)
						colors.push_back(utils::image::color::Cyan);
				}
				if (colors.empty())
					colors.push_back(utils::image::color::White);
				return colors;
			}
			//逻辑块的绘画位置
			utils::Index2D get_ui_position_for_basic_block(utils::Index2D logic_pos)
			{
				return {
					logic_pos.i*(m_ui_basic_block_size.i + m_ui_whitespace_size.i) + m_ui_whitespace_size.i,
					logic_pos.j*(m_ui_basic_block_size.j + m_ui_whitespace_size.j) + m_ui_whitespace_size.j
				};
			}
			//获得窗口当前的滚动位置
			utils::Index2D get_scroll_pos(HWND hwnd)
			{
				utils::Index2D scroll_pos;
				SCROLLINFO si{};
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_POS;
				utils_assert(GetScrollInfo(hwnd, SB_HORZ, &si)); scroll_pos.i = si.nPos;
				utils_assert(GetScrollInfo(hwnd, SB_VERT, &si)); scroll_pos.j = si.nPos;
				return scroll_pos;
			}
			//在棋盘中, 遍历每个基本块
			void iterator_basic_block_in_layout(
				std::function<void(utils::Index2D logic_pos, uint64_t block_addr, bool is_stop_loop)> meet_one_block)
			{
				for(int64_t j = 0; j < m_layout_size.j; j++)
				{
					for(int64_t i = 0; i < m_layout_size.i; i++)
					{
						if(!utils::map(m_layout.get_backward()).is_exist({i,j})) continue;

						const auto block_addr = utils::map(m_layout.get_backward()).get_existed_value({i,j});

						bool is_stop_loop = false;
						meet_one_block({i,j}, block_addr, is_stop_loop);

						if(is_stop_loop) return;
					}
				}
			}
			void handle_scroll_message(HWND hwnd, int nBar, WPARAM wParam, uint64_t line_step)
			{
				//还未完成数据的传递
				if(!m_is_inited) return;

				//获取滚动条信息
				SCROLLINFO si{};
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_ALL;
				utils_assert(GetScrollInfo(hwnd, nBar, &si));

				//处理滚动条消息
				int nPos = si.nPos;
				int nScrollCode = LOWORD(wParam);
				switch (nScrollCode)
				{
				case SB_LINEDOWN:
					nPos += line_step;
					if (nPos > si.nMax) nPos = si.nMax;
					break;

				case SB_LINEUP:
					nPos -= line_step;
					if (nPos < si.nMin) nPos = si.nMin;
					break;

				case SB_PAGEDOWN:
					nPos += si.nPage;
					if (nPos > si.nMax) nPos = si.nMax;
					break;

				case SB_PAGEUP:
					nPos -= si.nPage;
					if (nPos < si.nMin) nPos = si.nMin;
					break;

				case SB_THUMBTRACK:
					nPos = si.nTrackPos;
					break;

				default:
					break;
				}

				//设置滚动条的新位置
				SetScrollPos(hwnd, nBar, nPos, TRUE);

				//重绘窗口
				InvalidateRect(hwnd, NULL, FALSE);
			}
			//设置滚动条
			void set_scroll_bar(HWND hwnd)
			{
				if (utils::windows::ui::window::size::is_minimized(hwnd))
					return;

				//准备数据
				const auto client_size = utils::windows::ui::window::size::get_client_size(hwnd);
				
				//简单检查
				utils_assert(client_size.i > 0);
				utils_assert(client_size.j > 0);

				SCROLLINFO si{};
				si.cbSize = sizeof(si);
				si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_RANGE;
				si.nPage = client_size.i;
				si.nMin = 0;
				si.nMax = m_ui_total_size.i - 1;
				SetScrollInfo(hwnd, SB_HORZ, &si, FALSE);
				si.nPage = client_size.j;
				si.nMin = 0;
				si.nMax = m_ui_total_size.j - 1;
				SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
			}
		};
	}

	inline void draw(const basic_block::Graph & graph)
	{
		//简单检查
		utils_assert(!graph.get_basic_blocks().empty());
		
		//绘制图形
		utils::windows::ui::window::create::in_new_thread<inner::BasicBlockWindow>(
			u8"test.250227",
			0,
			WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME |
			WS_VSCROLL | WS_HSCROLL, 0, 0, 100, 100, u8"",
			&graph
		);
	}
}