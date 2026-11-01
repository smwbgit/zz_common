#pragma once

#include <stdint.h>
#include <compare>
#include <utils/integer_type.hpp>
#include <utils/iterator.hpp>

namespace utils::image::pixel
{
#pragma pack(push, 1)
	//图像中的一个像素
	class Pixel
	{
	public:
		uint8_t r{};
		uint8_t g{};
		uint8_t b{};

	public:
		Pixel()
		{
		}
		Pixel(uint8_t r, uint8_t g, uint8_t b)
			: r{r}, g{g}, b{b}
		{
		}

	public:
		//bool operator == (const Pixel &) const = default;
		std::strong_ordering operator <=> (const Pixel &) const = default;

		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & r;
			file & g;
			file & b;
		}
	};
#pragma pack(pop)

	//像素格式
	enum PixelFormat
	{
		RGB,
		BGR,
	};
}

namespace utils::image::pixel::change_pixel_format
{
	// 变换一些像素
	inline void in_some_pixels(Pixel* first, u8 count, PixelFormat from, PixelFormat to)
	{
		// 已是此格式
		if (from == to)
		{
			// 不再继续
			return;
		}

		// RGB 和 BGR 相互变换
		if (from == PixelFormat::BGR && to == PixelFormat::RGB ||
			from == PixelFormat::RGB && to == PixelFormat::BGR)
		{
			// 对于每个像素
			for(const auto i : utils::iterator_index(count))
			{
				// 进行变换
				std::swap(first[i].r, first[i].b);
			}

			// 完成
			return;
		}

		//报错
		utils_throw();
	}
}