#pragma once

#include <string>
#include <vector>
#include <utils/iterator.hpp>
#include <utils/map.hpp>

namespace utils::data_encoding::base64
{
	namespace inner
	{
		const inline char encode_table[] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		inline std::map<uint8_t, uint8_t> make_decode_table()
		{
			std::map<uint8_t, uint8_t> re;
			for (const auto i : utils::iterator_index(sizeof(encode_table)))
			{
				re[encode_table[i]] = (uint8_t)i;
			}

			//插入个'='以简化下边的代码逻辑
			re['='] = 0;
			return re;
		}
	}
	//编码
	inline void encode(_Out_ std::string & to, _In_ const std::vector<uint8_t> & from)
	{
		to.clear();
		to.reserve(((from.size()+2)/3)*3);

		auto push_back = [&](uint8_t c)
		{
			to.push_back(inner::encode_table[c]);
		};
		for (auto i : utils::iterator_index(from.size() / 3))
		{
			const uint8_t* _from = &from[i*3];
			push_back(((_from[0] >> 2) & 0x3f));
			push_back(((_from[1] >> 4) & 0x0f) | ((_from[0] & 0x3) << 4));
			push_back(((_from[2] >> 6) & 0x03) | ((_from[1] & 0xf) << 2));
			push_back(((_from[2] >> 0) & 0x3f));
		}

		//处理最后的1或2个字节
		if (from.size() % 3 == 1)
		{
			const uint8_t* _from = &from[from.size()/3*3];
			push_back(((_from[0] >> 2) & 0x3f));
			push_back(             0           | ((_from[0] & 0x3) << 4));
			to.push_back('=');
			to.push_back('=');
		}
		else if (from.size() % 3 == 2)
		{
			const uint8_t* _from = &from[from.size()/3*3];
			push_back(((_from[0] >> 2) & 0x3f));
			push_back(((_from[1] >> 4) & 0x0f) | ((_from[0] & 0x3) << 4));
			push_back(             0           | ((_from[1] & 0xf) << 2));
			to.push_back('=');
		}
	}
	//特化
	inline void encode(_Out_ std::string & to, _In_ const std::string & from)
	{
		std::vector<uint8_t> _from{from.begin(), from.end()};	//todo: not use _from
		return encode(to, _from);
	}
	//特化
	inline void encode(_Out_ std::vector<uint8_t> & to, _In_ const std::vector<uint8_t> & from)
	{
		std::string _to;	//todo: not use _to
		encode(_to, from);
		to.assign(_to.begin(), _to.end());
	}
	//解码
	inline void decode(_Out_ std::vector<uint8_t> & to, _In_ const std::vector<uint8_t> & from)
	{
		//简单检查
		utils_assert(from.size() % 4 == 0);

		//准备数据
		const static std::map<uint8_t, uint8_t> table = inner::make_decode_table();

		//循环每组(4个1组)
		to.clear();
		for (const auto i : utils::iterator_index(from.size()/4))
		{
			const uint8_t* _from = &from[i*4];

			const auto v0 = utils::map(table).get_existed_value(_from[0]);
			const auto v1 = utils::map(table).get_existed_value(_from[1]);
			const auto v2 = utils::map(table).get_existed_value(_from[2]);
			const auto v3 = utils::map(table).get_existed_value(_from[3]);
			
			to.push_back(((v0 & 0x3f) << 2) | ((v1 & 0x30) >> 4));
			to.push_back(((v1 & 0x0f) << 4) | ((v2 & 0x3C) >> 2));
			to.push_back(((v2 & 0x03) << 6) | ((v3 & 0x3f) >> 0));
		}

		//计算'='数量
		uint64_t equal_sign_n{};
		{
			for (const auto c : from)
				if (c == '=')
					equal_sign_n ++;

			//简单检查
			auto last = from.size()/4 - 1;
			switch(equal_sign_n)
			{
			case 0:
				break;
			case 1:
				utils_assert(from[last * 4 + 3] == '=');
				break;
			case 2:
				utils_assert(from[last * 4 + 2] == '=');
				utils_assert(from[last * 4 + 3] == '=');
				break;
			default:
				utils_throw();
			}
		}

		//根据'='数量删除一些数据
		if (equal_sign_n == 0)
		{
			;
		}
		else if (equal_sign_n == 1)
		{
			to.erase(to.end() - 1);
		}
		else if (equal_sign_n == 2)
		{
			to.erase(to.end() - 2, to.end());
		}
		else
			utils_throw();
	}

}
