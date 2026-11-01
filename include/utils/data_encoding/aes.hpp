#pragma once

#include <vector>
#include <array>
#include <openssl/evp.h>
#pragma comment(lib,"ws2_32.lib")	//needed by libcrypto_static.lib
#pragma comment(lib, "openssl/libcrypto_static.lib")
#include "utils/assert.hpp"
#include "utils/string.hpp"

//使用openssl来加密解密
namespace utils::data_encoding::aes::openssl
{
	//工作模式
	enum class Mode
	{
		ecb
	};
	
	//加密
	//from: 明文
	//to: 密文
	//key: 密钥
	//mode: 工作模式
	inline void encode(
		_Out_ std::vector<uint8_t> & to,
		_In_ const std::vector<uint8_t> & from,
		_In_ const std::vector<uint8_t> & key,
		Mode mode)
	{
		//简单检查
		utils_assert(mode == Mode::ecb);	//todo: 支持更多的模式
		utils_assert(key.size() == 16);		//todo: 支持更多的位数种类
		utils_assert(from.size() == 16);	//todo: 支持更多的输入位数种类

		const uint64_t block_size = key.size();	//todo: 看下是否key是否总是和分组大小相同

		EVP_CIPHER_CTX* x = EVP_CIPHER_CTX_new();
		utils_assert(x);

		utils_assert(1 == EVP_EncryptInit_ex2(x, EVP_aes_128_ecb(), key.data(), nullptr, nullptr));
//		utils_assert(1 == EVP_CIPHER_CTX_set_padding(x, EVP_PADDING_PKCS7));
		utils_assert(1 == EVP_CIPHER_CTX_set_padding(x, 0));

		//准备数据
		int from_size{};
		utils::computer_basic_function::store_this_integer(from_size, from.size());

		//准备数据
		int to_size{};
		{
			//准备的输出缓冲区的长度为: 输入长度 + 1个分组长度

			uint64_t _to_size = from_size + block_size;

			//简单检查: 加法是否溢出
			utils_assert(_to_size > (uint64_t)from_size);

			utils::computer_basic_function::store_this_integer(to_size, _to_size);
		}
		to.resize(to_size);

		utils_assert(1 == EVP_EncryptUpdate(x, to.data(), &to_size, from.data(), from_size));
		utils_assert(to_size <= to.size());

		int tail_size = (int)to.size() - to_size;
		utils_assert(1 == EVP_EncryptFinal_ex(x, to.data() + to_size, &tail_size));
		utils_assert(to_size + tail_size <= to.size());
		to.resize(to_size + tail_size);

		EVP_CIPHER_CTX_free(x);
	}

	//解密
	//from: 密文
	//to: 明文
	//key: 密钥
	//mode: 工作模式
	inline void decode(
		_Out_ std::vector<uint8_t> & to,
		_In_ const std::vector<uint8_t> & from,
		_In_ const std::vector<uint8_t> & key,
		Mode mode)
	{
		//简单检查
		utils_assert(mode == Mode::ecb);	//todo: 支持更多的模式
		utils_assert(key.size() == 16);		//todo: 支持更多的位数种类

		EVP_CIPHER_CTX* x = EVP_CIPHER_CTX_new();
		utils_assert(x);

		utils_assert(1 == EVP_DecryptInit_ex2(x, EVP_aes_128_ecb(), key.data(), nullptr, nullptr));
//		utils_assert(1 == EVP_CIPHER_CTX_set_padding(x, EVP_PADDING_PKCS7));
		utils_assert(1 == EVP_CIPHER_CTX_set_padding(x, 0));

		int from_size{};
		utils::computer_basic_function::store_this_integer(from_size, from.size());

		//准备输出缓冲
		{
			//准备个 {数据块大小 + 一个block_size} 大小的缓冲区, 确保其大小能放入一个int里

			uint64_t _to_size = 128/8 + from.size();
			int to_size{};
			utils::computer_basic_function::store_this_integer(to_size, _to_size);
			to.resize(to_size);
		}

		int to_size = (int)to.size();
		utils_assert(1 == EVP_DecryptUpdate(x, to.data(), &to_size, from.data(), from_size));
		utils_assert(to_size <= to.size());

		int temp = (int)to.size() - to_size;
		utils_assert(1 == EVP_DecryptFinal_ex(x, to.data() + to_size, &temp));
		utils_assert(to_size + temp <= to.size());
		to.resize(to_size + temp);

		EVP_CIPHER_CTX_free(x);
	}
}
#if 0
namespace utils::data_encoding::aes::undone
{
	//进行加密或解密
	class Translator
	{
	private:
		const uint32_t m_rcon[10] =
		{
			0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
			0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000,
		};
		const uint8_t m_sbox[256] =
		{
			0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
			0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
			0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
			0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
			0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
			0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
			0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
			0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
			0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
			0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
			0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
			0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
			0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
			0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
			0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
			0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
		};

	public:
		//加密
		//from: 明文
		//to: 密文
		//key: 密钥
		void encode(
			_Out_ std::vector<uint8_t> & to,
			_In_ const std::vector<uint8_t> & from,
			_In_ const std::vector<uint8_t> & key)
		{
			//简单检查
			utils_assert(key.size() == 16);		//todo: 支持更多的位数种类
			utils_assert(from.size() == 16);	//todo: 支持更多的输入位数种类

			//生成每轮密钥
			std::vector<uint8_t> round_keys;
			make_round_keys(round_keys, key, true);

			//输入数据每16字节一组, 目前只支持1组的.
			encode_one_block(to, from, round_keys);
		}

	private:
		//round_index: 使用哪一轮的密钥
		void add_round_key(_Inout_ uint8_t block[16], _In_ const std::vector<uint8_t> & round_keys, _In_ const uint64_t round_index)
		{
			for (
		}
		//对一组输入进行10轮加密
		void encode_one_block(
			_Out_ std::vector<uint8_t> & to,
			_In_ const std::vector<uint8_t> & from,
			_In_ const std::vector<uint8_t> & round_keys)
		{
			//简单检查
			utils_assert(round_keys.size() == 16*11);
			utils_assert(from.size() == 16);
			
			//复制输入
			uint8_t block[16]{};
			memmove(block, from.data(), 16);

			add_round_key(block, round_keys, 0);

			for (uint64_t i = 0; i < 9; i++)
			{
				replace_bytes(block);
				shift_rows(block);
				mix_columns(block);
				add_round_key(block, round_keys, i+1);
			}
			replace_bytes(block);
			shift_rows(block);
			add_round_key(block, round_keys, 10);
			
			//给出
			to.assign(block, block + 16);
		}
		//生成每轮密钥
		//key:主密钥
		//is_for_encode:为了加密还是解密
		void make_round_keys(
			_Out_ std::vector<uint8_t> & round_keys,
			_In_ const std::vector<uint8_t> & key,
			_In_ bool is_for_encode)
		{
			//简单检查
			utils_assert(key.size() == 16);

			//准备函数
			auto mix = [&](uint32_t x)
			{
				return utils::computer_basic_function::put_together<uint32_t>(
					m_sbox[utils::basic::bits::get_byte(x, 3)],
					m_sbox[utils::basic::bits::get_byte(x, 0)],
					m_sbox[utils::basic::bits::get_byte(x, 1)],
					m_sbox[utils::basic::bits::get_byte(x, 2)]
				);
			};

			//开辟空间
			round_keys.resize(4*4*11);	//总共11轮, 每轮4个uint32_t

			//第一轮
			memmove(&round_keys[0], &key[0], 16);

			//剩余轮
			for (uint64_t i  = 0; i < 10; i++)
			{
				//用上一轮的生成下一轮的

				uint32_t* w = (uint32_t*)&round_keys[i * 16];
				
				w[4] = w[0] ^ mix(w[3]) ^ m_rcon[i];
				w[5] = w[1] ^ w[4];
				w[6] = w[2] ^ w[5];
				w[7] = w[3] ^ w[6];
			}

			//如果为了解密, 反向给出
			if (false == is_for_encode)
			{
				auto org = std::move(round_keys);

				//开辟空间
				round_keys.resize(4*4*11);

				for (uint64_t i  = 0; i < 11; i++)
				{
					//复制每一轮

					auto from = &org[i * 16];
					auto to = &round_keys[(10-i) * 16];
					memmove(to, from, 16);
				}
			}
		}
	};
}
#endif