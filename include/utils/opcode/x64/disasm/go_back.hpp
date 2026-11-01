#pragma once

#include <utils/opcode/x64/disasm/modrm.hpp>

//向前查找
namespace utils::opcode::x64::disasm::go_back
{
	//查找一个紧邻的ModR/M(和它可能的SIB和Displacement)
	//return: 返回所有可能的ModR/M起始位置
	inline std::vector<uint64_t> get_modrm_positions(const utils::data & opcodes)
	{
		std::vector<uint64_t> positions;
		for(uint64_t i = 1; i <= utils::opcode::x64::disasm::modrm::get_max_byte_n();i++)
		{
			if (opcodes.size() < i) break;
			
			const auto b = opcodes[opcodes.size()-i];
			const auto n = utils::opcode::x64::disasm::modrm::get_byte_n(b);
			if(n < i) continue;	//需要的大小小于此位置的modr/m字节后边还剩下的字节
			positions.push_back(opcodes.size()-i);
		}
		return positions;
	}
}

namespace utils::opcode::x64::disasm::go_back::常见操作
{
	inline bool 是否为call指令(const utils::data & opcodes)
	{
		//E8 cd
		if (opcodes.size() >= 5)
		{
			if (opcodes[opcodes.size()-5] == 0xE8)
				return true;
		}

		//FF /2
		auto positions = utils::opcode::x64::disasm::go_back::get_modrm_positions(opcodes);
		for(const auto pos : positions)
		{
			if(pos == 0) continue;

			if(opcodes[pos-1] == 0xff)
			{
				if(utils::opcode::x64::disasm::modrm::field::get_opcode(opcodes[pos]) == 0x2)
				{
					return true;
				}
			}
		}
		return false;
	}
}