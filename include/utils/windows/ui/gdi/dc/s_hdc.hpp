#pragma once

#include <windows.h>
#include <utils/assert.hpp>
#include <utils/windows/ui/gdi/object/create_object.hpp>
#include <utils/windows/ui/gdi/object/Object.hpp>
#include <utils/windows/ui/rgb.hpp>
#include <utils/image/Image2D.hpp>
#include <utils/windows/ui/gdi/dc/Handle.hpp>
#include <utils/windows/ui/gdi/object/select.hpp>

namespace utils::windows::ui::gdi::dc
{
	class s_hdc
	{
	private:
		HDC m_hdc{};
	public:
		s_hdc(HDC hdc)
			: m_hdc{ hdc }
		{
		}
#pragma region 创建DC
		// 创建兼容DC
		auto create_compatible_dc() const
			-> utils::windows::ui::gdi::dc::Handle
		{
			// 创建
			HDC hdc = ::CreateCompatibleDC(m_hdc);

			// 简单检查
			utils_assert(hdc);

			// 加上自动删除功能
			utils::windows::ui::gdi::dc::Handle hdc_2{ hdc };

			// 给出
			return std::move(hdc_2);
		}
#pragma endregion
#pragma region 复制
		// 复制
		void copy_to(HDC to, utils::Index2D position_from, utils::Index2D position_to, utils::Index2D size) const
		{
			// 复制
			utils_assert(::BitBlt(
				to,
				position_to.i, position_to.j,
				size.i, size.j,
				m_hdc,
				position_from.i, position_from.j,
				SRCCOPY
			));
		}
		// 特化
		void copy_from(HDC from, utils::Index2D position_from, utils::Index2D position_to, utils::Index2D size) const
		{
			// 转发
			return s_hdc(from).copy_to(m_hdc, position_from, position_to, size);
		}
#pragma endregion
#pragma region 选入
		// 选入位图
		auto select(HBITMAP hBitmap) const
		{
			//转发
			return utils::windows::ui::gdi::object::select::bitmap(m_hdc, hBitmap);
		}
#pragma endregion
#pragma region 大小
	public:
		//获取逻辑大小
		utils::Index2D get_logic_size() const
		{
			return {
				GetDeviceCaps(m_hdc, HORZRES),
				GetDeviceCaps(m_hdc, VERTRES),
			};
		}
		//获取大小
		utils::Index2D get_size() const
		{
			//转发
			return get_logic_size();
		}
#pragma endregion
#pragma region 绘制上下文
	public:
		//设置文本颜色
		auto set_text_color(const utils::color::Color color) const
			-> utils::color::Color
		{
			//转为windows表示
			const auto _color = utils::windows::ui::rgb::from_color(color);

			//设置文本颜色
			const COLORREF old_color = ::SetTextColor(m_hdc, _color);

			//转为非windows表示
			const auto _old_color= utils::windows::ui::rgb::to_color(old_color);

			//返回原来的颜色
			return _old_color;
		}
		//设置背景模式
		int set_background_mode(const int mode) const
		{
			//设置背景模式
			const auto old_mode = ::SetBkMode(m_hdc, mode);

			//返回原来的模式
			return old_mode;
		}
		//设置背景透明
		int set_background_transparent(bool is_enable) const
		{
			//转发
			return set_background_mode(is_enable ? TRANSPARENT : OPAQUE);
		}
		//设置字体
		HFONT set_font(const HFONT font) const
		{
			//设置字体
			const auto old_font = (HFONT)::SelectObject(m_hdc, font);

			//返回原来的字体
			return old_font;
		}
#pragma endregion
#pragma region 绘制
#pragma region 直线
	public:
		//绘制直线
		void draw_straight_line(const utils::Index2D begin, const utils::Index2D end,
			const s8 pen_width, const utils::color::Color color) const
		{
			//直线长度为零
			if (begin == end)
			{
				//不再继续
				return;
			}

			//简单检查
			utils_assert(pen_width > 0);

			//创建画笔
			auto pen = utils::windows::ui::gdi::object::create_pen(PS_SOLID, pen_width, color);
			pen.select_me(m_hdc);

			//绘制直线
			utils_assert(::MoveToEx(m_hdc, begin.i, begin.j, NULL));
			utils_assert(::LineTo(m_hdc, end.i, end.j));

			//恢复旧画笔
			pen.select_old(m_hdc);
		}
		//绘制折线
		void draw_broken_line(const std::vector<utils::Index2D> & points, const s8 pen_width,
			const utils::color::Color color) const
		{
			//简单检查
			utils_assert(points.size() >= 2);

			//对于每对相邻点
			for(const auto & group : utils::iterator_neighbour(points))
			{
				//画一条直线
				draw_straight_line(group.first, group.second, pen_width, color);
			}
		}
#pragma endregion
#pragma region 矩形
	public:
		//绘制实心矩形
		void draw_rectangle_solid(const utils::Range2D & rect, const utils::color::Color color) const
		{
			//简单检查
			utils_assert(rect.size.i >= 0);
			utils_assert(rect.size.j >= 0);

			//区域为空
			if(rect.size.i == 0 || rect.size.j == 0)
			{
				//不再继续
				return;
			}

			//创建画刷
			const auto brush = utils::windows::ui::gdi::object::create_brush(color);

			//拼出位置
			RECT _rect{};
			_rect.left = rect.position.i;
			_rect.top = rect.position.j;
			_rect.right = rect.position.i + rect.size.i;
			_rect.bottom = rect.position.j + rect.size.j;

			//绘制
			utils_assert(::FillRect(m_hdc, &_rect, brush));
		}
		//特化
		void draw_rectangle_solid(const utils::color::Color color) const
		{
			//获取大小
			const auto size = get_size();

			//绘制区域
			const utils::Range2D rect{{}, size};

			//转发
			return draw_rectangle_solid(rect, color);
		}
		//绘制矩形边框
		//note: 边框为内边框, i.e. 绘制的边框，均在矩形内部
		void draw_rectangle_border(const utils::Range2D & rect, const s8 border_width,
			const utils::color::Color color) const
		{
			//简单检查
			utils_assert(rect.size.i > 0);
			utils_assert(rect.size.j > 0);
			utils_assert(border_width > 0);
			utils_assert(rect.size.i >= border_width * 2);
			utils_assert(rect.size.j >= border_width * 2);

	#if 0
			//创建画笔
			namespace gdi = utils::windows::ui::window::paint::gdi;
			auto pen = gdi::create_pen(PS_SOLID, border_width, RGB(color.r, color.g, color.b));
			pen.select_me(hdc);

			//选择空心画刷
			auto brush = gdi::get_stock_brush(HOLLOW_BRUSH);
			brush.select_me(hdc);

			//绘制空心矩形
			gdi::rectangle(hdc, x, y, x+w, y+h);
		
			//恢复旧画刷
			brush.select_old(hdc);

			//恢复旧画笔
			pen.select_old(hdc);
	#endif
			//todo 看看还有没有更简单的办法

			//绘制空心矩形
			for(s8 i = 0; i < border_width; i++)
			{
				const auto x = rect.position.i + i;
				const auto y = rect.position.j + i;
				const auto w = rect.size.i - i * 2;
				const auto h = rect.size.j - i * 2;
				draw_straight_line({x, y}, {x + w, y}, 1, color);
				draw_straight_line({x + w - 1, y}, {x + w - 1, y + h}, 1, color);
				draw_straight_line({x + w - 1, y + h - 1}, {x, y + h - 1}, 1, color);
				draw_straight_line({x, y + h - 1}, {x, y}, 1, color);
			}
		}
#pragma endregion
#pragma region 文本
	public:
		//绘制文本
		void draw_text_transparent(const std::u8string & text, const utils::Index2D position,
			const HFONT font, const utils::color::Color color) const
		{
			//文本为空
			if(text.empty())
			{
				//不再继续
				return;
			}

			//设置字体
			const auto old_font = set_font(font);

			//设置文本颜色
			const auto old_color = set_text_color(color);

			//设置背景透明
			const auto old_bk_mode = set_background_mode(TRANSPARENT);

			//文本转为u16格式
			const auto text_u16 = utils::text::encoding::utf8_to_utf16(text);

			//绘制文本
			utils_assert(::TextOutW(m_hdc, position.i, position.j,
				(LPCWSTR)text_u16.c_str(), (int)text_u16.length()));

			//恢复原始状态
			set_font(old_font);
			set_text_color(old_color);
			set_background_mode(old_bk_mode);
		}
#pragma endregion
#pragma endregion
	};
}