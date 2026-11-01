#pragma once

#include <utils/map.hpp>
#include <utils/data.hpp>
#include <utils/basic/change_bit_order.hpp>

namespace utils::data_encoding::crc32
{
	//表示一种crc32标准
	struct s_crc32_param
	{
		uint32_t polynomial;		//多项式
		uint32_t initial_value;		//初始值
		uint32_t final_xor_value;	//最终异或值
		bool is_reflect_input;		//输入反转
		bool is_reflect_output;		//输出反转
	};
	//各种crc32标准
	namespace standard
	{
		//refer：https://www.toolhelper.cn/DigestAlgorithm/CRC

		static s_crc32_param Normal = {0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true};
		static s_crc32_param MPEG2 = {0x04C11DB7, 0xFFFFFFFF, 0x00000000, false, false};
		static s_crc32_param Castagnoli = {0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true};
	}
    namespace inner
    {
		//生成CRC32查找表
		inline std::vector<uint32_t> make_table(uint32_t polynomial)
		{
			std::vector<uint32_t> table(256);

			for (uint32_t i = 0; i < 256; ++i) {
				uint32_t crc = i << 24;
				for (int j = 0; j < 8; ++j) {
					if (crc & 0x80000000)
						crc = (crc << 1) ^ polynomial;
					else
						crc <<= 1;
				}
				table[i] = crc;
			}
			return table;
		}
		//获取CRC32查找表
		inline const std::vector<uint32_t>& get_table(uint32_t polynomial)
		{
			static std::map<uint32_t, std::vector<uint32_t>> tables;

			if(!utils::map(tables).is_exist(polynomial))
				tables[polynomial] = make_table(polynomial);

			return tables[polynomial];
		}
    }
	//计算crc32
	inline uint32_t encode(utils::data_view data_view, s_crc32_param crc32_param)
	{
        const auto & table = inner::get_table(crc32_param.polynomial);

        uint32_t crc = crc32_param.initial_value;
    
        for (const auto byte : data_view)
        {
            uint8_t _byte = byte;
            if (crc32_param.is_reflect_input)
                utils::basic::change_bit_order(_byte);
			crc = (crc << 8) ^ table[(crc >> 24) ^ _byte];
        }

        if (crc32_param.is_reflect_output)
            utils::basic::change_bit_order(crc);
    
        return crc ^ crc32_param.final_xor_value;
	}
}