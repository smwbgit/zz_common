#pragma once

#include <utils/image/pixel.hpp>
#include <utils/Range2D.hpp>
#include <utils/iterator.hpp>

namespace utils::image
{
	//一张2D图像
	class Image2D
	{
		//便利声明
	public:
		using PixelFormat = utils::image::pixel::PixelFormat;

	public:
		//仅做为,对外界的提示作用.不影响,本类里,其他函数的,任何行为.(除了,在生成子图像或类似场景里,会因此多一个,复制语句)
		PixelFormat m_pixel_format{ PixelFormat::RGB };

	private:
		//图片大小
		utils::Index2D m_size;
		//像素数组
		std::vector<utils::image::pixel::Pixel> m_pixels;

	public:
		Image2D()
		{
		}
		explicit Image2D(utils::Index2D size)
		{
			//设置大小
			set_size(size);
		}
		Image2D(const Image2D &) = default;
		Image2D & operator=(const Image2D &) = default;
		Image2D(Image2D &&) = default;
		Image2D & operator=(Image2D &&) = default;
		bool operator == (const Image2D &) const = default;
	public:
		//设置像素
		void set_pixel(const utils::Index2D position, const utils::image::pixel::Pixel & pixel)
		{
			//简单检查
			utils_assert(position.is_in(m_size));

			//保存
			m_pixels[get_index_1D(position)] = pixel;
		}
		//获取像素
		const utils::image::pixel::Pixel & get_pixel(const utils::Index2D position) const
		{
			//简单检查
			utils_assert(position.is_in(m_size));

			//给出
			return m_pixels[get_index_1D(position)];
		}
		//获取大小
		utils::Index2D get_size() const
		{
			return m_size;
		}
		//设置大小
		void set_size(const utils::Index2D size)
		{
			//简单检查
			utils_assert(size.i >= 0);
			utils_assert(size.j >= 0);
			utils_assert(size.i > 0 || size.j > 0 || size.i == 0 && size.j == 0);

			m_size = size;
			m_pixels.resize(size.i * size.j);
		}
		//获取数据指针
		const uint8_t* get_data() const
		{
			//简单检查
			utils_assert(!m_pixels.empty());

			return (const uint8_t*)m_pixels.data();
		}
		//特化
		const uint8_t* get_const_data() const
		{
			return get_data();
		}
		//特化
		uint8_t* get_data()
		{
			return (uint8_t*)get_const_data();
		}
		//特化
		const uint8_t* get_data(utils::Index2D position) const
		{
			//for debug
			{
				const auto a = get_index_1D(position);
				utils_assert(m_size.i*position.j+position.i == a);
			}

			return get_data() + get_index_1D(position)*3;
		}
		//特化
		const uint8_t* get_const_data(utils::Index2D position) const
		{
			return get_data(position);
		}
		//特化
		uint8_t* get_data(utils::Index2D position)
		{
			return (uint8_t*)get_const_data(position);
		}
		//获取子图片
		//note: 设定的获取范围需要在图片范围内
		Image2D get_sub_image(const utils::Range2D & rect) const
		{
			//简单检查
			utils_assert(rect.is_in(utils::Range2D{{0,0}, m_size}));

			//准备数据
			Image2D sub_image{rect.size};

			//对于每一行
			for(const auto j : utils::iterator_index(rect.size.j))
			{
				//当前行首
				const auto position_in_small = utils::Index2D{0, j};
				const auto position_in_big = rect.position + position_in_small;

				//一行的字节大小
				const auto size = rect.size.i * 3;
				
				//数据位置
				const auto data_in_small = sub_image.get_data(position_in_small);
				const auto data_in_big = get_data(position_in_big);

				//复制一行
				memmove(data_in_small, data_in_big, size);
			}

			//保持像素格式
			sub_image.m_pixel_format = m_pixel_format;

			//给出
			return std::move(sub_image);
		}
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_size;
			file & m_pixels;
		}
		//获取在像素数组中的一维索引
		uint64_t get_index_1D(utils::Index2D position) const
		{
			return position.index_1D(m_size);
		}
	};
}
