#pragma once

#include <utils/windows/ui/screenshot/ScreenshotCache.hpp>
#include <utils/windows/ui/auto_operation/SmallImage.hpp>
#include <utils/windows/ui/simulate/simple.hpp>
#include <utils/windows/ui/mouse.hpp>
#include <utils/file_format/bmp.hpp>
#include <utils/text/filename/save_screenshot.hpp>
#include <utils/image/md5.hpp>

namespace utils::windows::ui::auto_operation
{
	//提供基础函数
	class AutoOperation_1
	{
	private:
		utils::windows::ui::screenshot::ScreenshotCache m_screenshot_cache;
	public:
		//抓取最新的桌面画面
		void update_screenshot()
		{
			m_screenshot_cache.update_value();
		}
		void save_screenshot(const std::u8string & filename)
		{
			const auto & screenshot = get_screenshot();

			utils::file_format::bmp::save(screenshot, filename);
		}
		struct save_screenshot__params
		{
			bool is_need_update{};
			std::u8string dir_name;
		};
		void save_screenshot(const save_screenshot__params & params)
		{
			if(params.is_need_update)
				update_screenshot();
			
			const auto filename = utils::text::filename::save_screenshot::make_by_dirname(params.dir_name);
			save_screenshot(filename);
		}
		const utils::image::Image2D & get_screenshot()
		{
			return m_screenshot_cache.get_value();
		}
		void click(const utils::Index2D position)
		{
			//转发
			click_mouse(position, true);
		}
		void right_click(const utils::Index2D position)
		{
			//转发
			click_mouse(position, false);
		}
		//模拟鼠标拖动
		void drag_mouse(const utils::Index2D from, const utils::Index2D to)
		{
			//鼠标当前位置
			const auto mouse_old_position = utils::windows::ui::mouse::get_position();

			//模拟鼠标拖动
			utils::windows::ui::simulate::simple::real::send_input::mouse::drag(from, to);

			//恢复鼠标位置
			utils::windows::ui::mouse::set_position(mouse_old_position);

			//休息一会儿
			Sleep(1000);
		}
		void type(BYTE vkey)
		{
			utils::windows::ui::simulate::simple::real::send_input::keyboard::type(vkey);
			Sleep(1000);
		}
		//是否存在指定图片
		//position: 小图片位置的额外偏移
		bool is_exist(const SmallImage & small_image, const utils::Index2D position)
		{
			//获取屏幕图片
			const auto & screenshot = get_screenshot();

			//要对比的区域
			const auto rect = [&]()
				-> utils::Range2D
			{
				//小图片的矩形
				const auto & rect = small_image.m_rect;

				//新的位置，添加上额外偏移
				const auto _position = rect.position + position;

				//新的大小，保持不变
				const auto & _size = rect.size;

				//给出
				return {_position, _size};
			}();

			//todo 看看是否需要，当有原始图片时，使用原始图片，来比较。比如，是否这样会更快

			//获取对应位置的图片
			const auto bmp = screenshot.get_sub_image(rect);

			//计算它的md5
			const auto md5 = utils::image::md5::get(bmp);

			//给出
			return md5 == small_image.m_md5;
		}
		//特化
		bool is_exist(const SmallImage & small_image)
		{
			//转发
			return is_exist(small_image, {});
		}
	private:
		void click_mouse(const utils::Index2D position, bool is_left_key)
		{
			const auto mouse_old_position = utils::windows::ui::mouse::get_position();
			if(is_left_key)
				utils::windows::ui::simulate::simple::real::send_input::mouse::click(position);
			else
				utils::windows::ui::simulate::simple::real::send_input::mouse::right_click(position);
			utils::windows::ui::mouse::set_position(mouse_old_position);
			Sleep(1000);
		}
	};
	//添加操作函数
	class AutoOperation_2 : public AutoOperation_1
	{
	public:
		//通过用鼠标点击一个图片，来让它消失
		//return: 是否存在并点击了它 
		bool disappear_it_by_click(const SmallImage & small_image)
		{
			//存在小图片
			if(is_exist(small_image))
			{
				//小图片中心
				const auto center_position = small_image.m_rect.get_center_position();

				//点击
				click(center_position);

				//给出：已点击
				return true;
			}

			//给出：未点击
			return false;
		}
	};
	//自动操作电脑
	using AutoOperation = AutoOperation_2;
}