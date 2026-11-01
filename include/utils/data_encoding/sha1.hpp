#pragma once

#include <utils/data.hpp>

namespace utils::data_encoding::sha1
{
	//ai生成后整理
	class Encoder
	{
	private:
		uint32_t state[5]{};
		uint64_t total_bits{};
		uint8_t buffer[64]{};
		size_t buffer_size{};

	public:
		//添加数据，可分批给数据
		void add_data(utils::data_view data)
		{
			const uint8_t* _data = data.addr();
			size_t len = data.size();

			size_t i = 0;
			while (i < len) {
				buffer[buffer_size++] = _data[i++];
				if (buffer_size == 64) {
					process_block(buffer);
					total_bits += 512;
					buffer_size = 0;
				}
			}
		}
		//结束计算，返回 20 字节的结果
		//note: get_result仅可调用一次
		//todo: 看看能不能去掉此限制
		utils::data get_result()
		{
			uint64_t total = total_bits + (buffer_size * 8);
        
			// 填充
			buffer[buffer_size++] = 0x80;
			if (buffer_size > 56) {
				while (buffer_size < 64) buffer[buffer_size++] = 0;
				process_block(buffer);
				buffer_size = 0;
			}
			while (buffer_size < 56) buffer[buffer_size++] = 0;
        
			// 追加长度（大端序）
			for (int i = 7; i >= 0; --i) {
				buffer[buffer_size++] = (total >> (i * 8)) & 0xFF;
			}
			process_block(buffer);
        
			// 输出 20 字节
			utils::data digest;
			digest.resize(20);
			for (int i = 0; i < 5; ++i) {
				digest[i*4]   = (state[i] >> 24) & 0xFF;
				digest[i*4+1] = (state[i] >> 16) & 0xFF;
				digest[i*4+2] = (state[i] >> 8)  & 0xFF;
				digest[i*4+3] = (state[i])       & 0xFF;
			}

			//重置
			reset();

			return digest;
		}
	private:
		//重置
		void reset()
		{
			state[0] = 0x67452301;
			state[1] = 0xEFCDAB89;
			state[2] = 0x98BADCFE;
			state[3] = 0x10325476;
			state[4] = 0xC3D2E1F0;
			total_bits = 0;
			buffer_size = 0;
		}
		static uint32_t rotl(uint32_t x, uint32_t n)
		{
			return (x << n) | (x >> (32 - n));
		}
		void process_block(const uint8_t* block)
		{
			uint32_t w[80];
			for (int i = 0; i < 16; ++i) {
				w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) 
					 | (block[i*4+2] << 8) | block[i*4+3];
			}
			for (int i = 16; i < 80; ++i) {
				w[i] = rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
			}
        
			uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
        
			for (int i = 0; i < 80; ++i) {
				uint32_t f, k;
				if (i < 20) {
					f = (b & c) | ((~b) & d);
					k = 0x5A827999;
				} else if (i < 40) {
					f = b ^ c ^ d;
					k = 0x6ED9EBA1;
				} else if (i < 60) {
					f = (b & c) | (b & d) | (c & d);
					k = 0x8F1BBCDC;
				} else {
					f = b ^ c ^ d;
					k = 0xCA62C1D6;
				}
				uint32_t temp = rotl(a, 5) + f + e + k + w[i];
				e = d; d = c; c = rotl(b, 30); b = a; a = temp;
			}
        
			state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
		}
	};

	//简便函数: 获取sha1
	inline utils::data encode(utils::data_view data)
	{
		//送入数据
		Encoder encoder;
		encoder.add_data(data);

		//获取结果
		const auto sha1 = encoder.get_result();

		//给出
		return sha1;
	}
}