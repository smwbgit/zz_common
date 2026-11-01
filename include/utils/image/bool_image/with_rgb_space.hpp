#pragma once

#include <utils/image/bool_image/BoolImage.hpp>
#include <utils/image/Image2D.hpp>
#include <utils/color/Color.hpp>
#include <utils/color/distance.hpp>

namespace utils::image::bool_image::with_rgb_space::from_rgb_image
{
	//把彩色图片转为布尔图片（二值化‌）
	//foreground_color: 前景色，会变为true
	//background_color: 背景色，会变为false
	//max_error_allowed: 最大允许的误差, 从0到1。
	//					 0表示，只有当精确匹配前景色时，才为true像素
	//					 0.5表示，只有距离前景色更近时，才为true像素
	inline BoolImage all_color(
		const utils::image::Image2D & image,
		const utils::color::Color foreground_color,
		const utils::color::Color background_color,
		const double max_error_allowed)
	{
		//简单检查
		utils_assert(foreground_color != background_color);

		//图片大小
		const auto image_size = image.get_size();

		//建立一个一样大的布尔图片
		BoolImage bool_image{image_size};

		//对于每个像素位置
		for(const auto position : image_size)
		{
			//获取彩色图片中的像素值
			const auto pixel = image.get_pixel(position);

			//生成相应的布尔值
			const auto bool_pixel = [&]()
			{
				//像素颜色和前景色的距离
				const auto distance_for_foreground_color = utils::color::distance::
					get(pixel, foreground_color);

				//像素颜色和背景色的距离
				const auto distance_for_background_color = utils::color::distance::
					get(pixel, background_color);

				//颜色距离之和
				const auto distance_sum = distance_for_foreground_color + distance_for_background_color;

				//简单检查
				utils_assert(distance_sum > 0);

				//前景色的距离，在总距离的占比
				const auto foreground_color_error = distance_for_foreground_color / distance_sum;

				//在允许的误差范围内
				if(foreground_color_error < max_error_allowed)
				{
					//给出：true 像素
					return true;
				}
				//否则
				else
				{
					//给出：false 像素
					return false;
				}
			}();

			//保存
			bool_image.set_pixel(position, bool_pixel);
		}

		//给出
		return bool_image;
	}
	//把彩色图片转为布尔图片
	//note: 彩色图片中，只允许存在 前景色和背景色 两种颜色
	inline BoolImage two_color(
		const utils::image::Image2D & image,
		const utils::color::Color foreground_color = utils::color::Red,
		const utils::color::Color background_color = utils::color::Black)
	{
		//简单检查
		utils_assert(foreground_color != background_color);

		//图片大小
		const auto image_size = image.get_size();

		//建立一个一样大的布尔图片
		BoolImage bool_image{image_size};

		//对于每个像素位置
		for(const auto position : image_size)
		{
			//获取彩色图片中的像素值
			const auto pixel = image.get_pixel(position);

			//生成相应的布尔值
			const auto bool_pixel = [&]()
			{
				//前景色, 转为 true 像素
				if(pixel == foreground_color)
					return true;

				//背景色, 转为 false 像素
				if(pixel == background_color)
					return false;

				//简单检查
				utils_throw();
			}();

			//保存
			bool_image.set_pixel(position, bool_pixel);
		}

		//给出
		return bool_image;
	}
}
namespace utils::image::bool_image::with_rgb_space
{
	//把布尔图片转为彩色图片
	inline utils::image::Image2D to_rgb_image(
		const BoolImage & bool_image,
		const utils::color::Color foreground_color = utils::color::Red,
		const utils::color::Color background_color = utils::color::Black)
	{
		//图片大小
		const auto image_size = bool_image.get_size();

		//准备一个一样大小的彩色图片
		utils::image::Image2D rgb_image{ image_size };

		//对于每个像素位置
		for(const auto position : image_size)
		{
			//获取布尔像素
			const auto bool_pixel = bool_image.get_pixel(position);

			//是true像素
			if(bool_pixel)
			{
				//彩色图片里，设置为前景色
				rgb_image.set_pixel(position, foreground_color);
			}
			//是false像素
			else
			{
				//彩色图片里，设置为背景色
				rgb_image.set_pixel(position, background_color);
			}
		}

		//给出
		return rgb_image;
	}
}