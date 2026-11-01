#pragma once

#include <utils/Range2D.hpp>

//布尔图片
namespace utils::image::bool_image
{
	//一张布尔图片
	class BoolImage
	{
	private:
		//图片大小
		utils::Index2D m_size;
		//像素数组
		std::vector<bool> m_pixels;

	public:
		BoolImage()
		{
		}
		explicit BoolImage(utils::Index2D size)
			: m_size{ size }
		{
			m_pixels.resize(m_size.i * m_size.j);
		}
		BoolImage(const BoolImage &) = default;
		BoolImage & operator=(const BoolImage &) = default;
		BoolImage(BoolImage &&) = default;
		BoolImage & operator=(BoolImage &&) = default;
		bool operator == (const BoolImage &) const = default;
	public:
		//设置像素
		void set_pixel(const utils::Index2D position, const bool pixel)
		{
			//简单检查
			utils_assert(position.is_in(m_size));

			//保存
			m_pixels[get_index(position)] = pixel;
		}
		//获取像素
		bool get_pixel(const utils::Index2D position) const
		{
			//简单检查
			utils_assert(position.is_in(m_size));

			//给出
			return m_pixels[get_index(position)];
		}
		//获取大小
		utils::Index2D get_size() const
		{
			//给出
			return m_size;
		}
		//获取子图片
		//note: 设定的获取范围需要在图片范围内
		BoolImage get_sub_image(const utils::Range2D & rect) const
		{
			BoolImage sub_image{rect.size};
			for(const auto position_in_small : rect.size)
			{
				const auto position_in_big = rect.position + position_in_small;
				const auto pixel = get_pixel(position_in_big);
				sub_image.set_pixel(position_in_small, pixel);
			}
			return sub_image;
		}
	private:
		//获取在像素数组中的索引
		uint64_t get_index(utils::Index2D position) const
		{
			return position.index_1D(m_size);
		}
	};
}