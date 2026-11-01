#pragma once

#include <utils/image/Image2D.hpp>
#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>

namespace utils::file_format::bmp
{
	namespace inner
	{
		#pragma pack(push, 1)
		struct BMPHeader {
			uint16_t file_type;
			uint32_t file_size;
			uint16_t reserved1;
			uint16_t reserved2;
			uint32_t offset_data;
		};

		struct BMPInfoHeader {
			uint32_t size;
			int32_t width;
			int32_t height;
			uint16_t planes;
			uint16_t bit_count;
			uint32_t compression;
			uint32_t size_image;
			int32_t x_pixels_per_meter;
			int32_t y_pixels_per_meter;
			uint32_t colors_used;
			uint32_t colors_important;
		};
		#pragma pack(pop)
	}

	inline void save(const uint8_t* rgb_data, uint64_t width, uint64_t height, const std::u8string & filename)
	{
		using inner::BMPHeader;
		using inner::BMPInfoHeader;

		//简单检查
		utils_assert(width);
		utils_assert(height);

		utils::FileWriter writer{filename};

		BMPHeader header{};
		BMPInfoHeader info_header{};
		
		//填写固定值
		header.file_type = 0x4D42;
		header.offset_data = 54;
		info_header.size = 40;
		info_header.planes = 1;
		info_header.bit_count = 24;
	
		// 计算对齐后的每行字节数
		uint64_t row_size = ((width * 3 + 3) / 4) * 4;
		uint64_t padding_size = row_size - width * 3;

		info_header.width = width;
		info_header.height = -utils::basic::store_in<int32_t>(height);	// 负值表示顶点在上方
		info_header.size_image = row_size * height;
		header.file_size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + info_header.size_image;

		//bmp颜色顺序是倒着的
		std::vector<uint8_t> data(info_header.size_image);
		for (uint64_t y = 0; y < height; y++)
		{
			for (uint64_t x = 0; x < width; x++)
			{
				uint64_t src_index = y * (width * 3) + x * 3;
				uint64_t dst_index = y * row_size + x * 3;
				data[dst_index + 0] = rgb_data[src_index + 2]; // B
				data[dst_index + 1] = rgb_data[src_index + 1]; // G
				data[dst_index + 2] = rgb_data[src_index + 0]; // R
			}
		}
	
		writer.write(reinterpret_cast<char*>(&header), sizeof(header));
		writer.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));
		writer.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
	//特化
	inline void save(const utils::image::Image2D & image, const std::u8string & filename)
	{
		auto size = image.get_size();
		utils::file_format::bmp::save(image.get_const_data(), size.i, size.j, filename);
	}
	//读取bmp文件
	inline utils::image::Image2D load(const std::u8string & filename)
	{
		using inner::BMPHeader;
		using inner::BMPInfoHeader;

		//读取头信息
		utils::FileReader input{filename};
		BMPHeader file_header{};
		BMPInfoHeader info_header{};
		input.read(file_header);
		input.read(info_header);
		
		//简单检查
		utils_assert(file_header.file_type == 0x4D42);	//BMP文件
		utils_assert(file_header.offset_data == 54);
		utils_assert(info_header.bit_count == 24);		//仅支持24位BMP
		utils_assert(info_header.colors_important == 0);
		utils_assert(info_header.colors_used == 0);
		utils_assert(info_header.compression == 0);		//不支持压缩格式
		utils_assert(info_header.planes == 1);
		utils_assert(info_header.size == 40);
		utils_assert(info_header.width != 0);
		utils_assert(info_header.height != 0);
		//utils_assert(info_header.x_pixels_per_meter == 0);
		//utils_assert(info_header.y_pixels_per_meter == 0);
		utils_assert(sizeof(BMPHeader) + sizeof(BMPInfoHeader) + info_header.size_image == file_header.file_size);

		//计算行对齐填充
		const uint64_t width = info_header.width;
		const uint64_t height = abs(info_header.height);
		const uint64_t row_size = ((width * 3 + 3) / 4) * 4;
		const uint64_t padding_size = row_size - width * 3;
		
		//简单检查
		utils_assert(row_size * height == info_header.size_image);

		//读取像素数据
		std::vector<uint8_t> pixels;
		input.read(pixels, row_size * height);

		//进行数据转换：BGR转RGB，去掉行填充，上下翻转
		utils::image::Image2D image{{width, height}};
		auto rgb_data = image.get_data();
		for (uint64_t y = 0; y < height; y++)
		{
			uint64_t src_y = (info_header.height > 0) ? (height - 1 - y) : y;
			for (uint64_t x = 0; x < width; x++)
			{
				uint64_t src_index = src_y * row_size + x * 3;
				uint64_t dst_index = y * (width * 3) + x * 3;
				rgb_data[dst_index + 0] = pixels[src_index + 2]; // R
				rgb_data[dst_index + 1] = pixels[src_index + 1]; // G
				rgb_data[dst_index + 2] = pixels[src_index + 0]; // B
			}
			//跳过行填充
			for (uint64_t x = 0; x < padding_size; x++)
			{
				uint64_t src_index = src_y * row_size + width * 3 + x;

				//简单检查: 填充字节为0
				utils_assert(pixels[src_index] == 0);
			}
		}
		return image;
	}
}