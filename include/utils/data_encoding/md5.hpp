#pragma once

#include <array>
#include <vector>
#include <utils/assert.hpp>
#include <utils/data.hpp>
#include <utils/integer_type.hpp>

namespace utils::data_encoding::md5
{
	using MD5Value = std::array<u1, 16>;

	//编码
	//k: 指定64个数, 或使用默认值
	//todo: add 快速版
	inline MD5Value encode(
		_In_ utils::data_view from,
		_In_ const std::vector<u4> & k = {})
	{
		//简单检查
		utils_assert(!from.empty());

		//准备函数: 循环左移
		auto LEFTROTATE = [](u4 x, u4 c) -> u4
		{
			return (x << c) | (x >> (32 - c));
		};

		//准备数据
		static const std::vector<u4> default_k = {
			0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
			0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
			0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
			0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
			0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
			0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
			0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
			0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
			0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
			0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
			0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
			0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
			0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
			0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
			0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
			0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
		};
		static const std::vector<u4> r = {
			7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
            5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
            4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
            6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
		};

		//准备数据: k
		const auto & _k = [&]() -> const std::vector<u4> &
		{
			//或使用默认值, 或使用传入的k

			if (k.empty())
				return default_k;
			else
				return k;
		}();

		//准备数据
		u4 h[4] = 
		{
			0x67452301,
			0xefcdab89,
			0x98badcfe,
			0x10325476
		};

		//补齐数据
		std::vector<u1> blocked_data;
		{
			//1. 先添加'1', 再添加若干个'0', 直到 长度的位数 % 512 == 448
			//2. 然后, 添加一个u8, 值为 原始长度的位数
		
			//补齐后的长度
			u8 _size{};
			for (_size = from.size() + 1; _size % (512 / 8) != 448 / 8; _size++)
				;

			//填充数据
			blocked_data.resize(_size + 8, 0);
			memmove(blocked_data.data(), from.addr(), from.size());
			blocked_data[from.size()] = 0x80;	//填充'1'
			*(u8*)&blocked_data[_size] = from.size() * 8;	//填充原始数据的位数
		}

		//循环每个512位的块
		for (u8 offset = 0; offset < blocked_data.size(); offset += (512 / 8))
		{
			//变换输入数据的类型
			u4 w[16]{};
			for (u8 i = 0; i < 16; i++)
				w[i] = *(u4*)&blocked_data[offset + i * 4];

			//准备数据
			u4 a = h[0];
			u4 b = h[1];
			u4 c = h[2];
			u4 d = h[3];
		
			//主循环
			for (u8 i = 0; i < 64; i++)
			{
				u4 f{};
				u8 g{};

				if (i < 16)
				{
					f = (b & c) | ((~b) & d);
					g = i;
				}
				else if (i < 32)
				{
					f = (d & b) | ((~d) & c);
					g = (5 * i + 1) % 16;
				}
				else if (i < 48)
				{
					f = b ^ c ^ d;
					g = (3 * i + 5) % 16;
				}
				else
				{
					f = c ^ (b | (~d));
					g = (7 * i) % 16;
				}

				u4 temp = d;
				d = c;
				c = b;
				b = b + LEFTROTATE((a + f + _k[i] + w[g]), r[i]);
				a = temp;
			}

			//保存结果
			h[0] += a;
			h[1] += b;
			h[2] += c;
			h[3] += d;
		}

		//拼出md5值
		MD5Value md5_value{};
		std::copy((u1*)h, (u1*)h + 16, md5_value.begin());

		//给出
		return md5_value;
	}
}
