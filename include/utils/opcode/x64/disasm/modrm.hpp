#pragma once

//ModR/M字节
namespace utils::opcode::x64::disasm::modrm
{
	namespace field
	{
		//获得mod字段
		inline uint8_t get_mod(const uint8_t modrm)
		{
			return (modrm & 0xc0) >> 6;
		}
		//获得reg/opcode字段
		inline uint8_t get_reg(const uint8_t modrm)
		{
			return (modrm & 0x38) >> 3;
		}
		//获得reg/opcode字段
		inline uint8_t get_opcode(const uint8_t modrm)
		{
			return get_reg(modrm);
		}
		//获得r/m字段
		inline uint8_t get_rm(const uint8_t modrm)
		{
			return modrm & 0x7;
		}
	}

	//'get_byte_n()'函数最大返回值
	inline uint64_t get_max_byte_n()
	{
		return 6;
	}

	//从data[0]开始读取一个完整的modrm(包括可能得sib和displacement), 返回它的总长度(包括modrm字节)
	//return: 如果size不够大, 返回空
	inline std::optional<uint64_t> get_byte_n(const uint8_t * data, uint64_t size)
	{
		if(size == 0) return {};
		const auto modrm = data[0];

		auto check = [&](const uint64_t _size) -> std::optional<uint64_t>
		{
			if (_size <= size) return _size;
			else return {};
		};
		namespace field = utils::opcode::x64::disasm::modrm::field;
		switch (field::get_mod(modrm))
		{
		case 0:
			switch(field::get_rm(modrm))
			{
			case 0: case 1: case 2: case 3: case 6: case 7: return check(1);
			case 4:
			{
				if(size <= 1) return {};
				const auto sib = data[1];

				if(sib == 0x25) return check(6);
				else return check(2);
			}
			case 5: return check(5);
			}
			utils_throw();
		case 1:
			switch(field::get_rm(modrm))
			{
			case 0: case 1: case 2: case 3: case 5: case 6: case 7: return check(2);
			case 4: return check(3);
			}
			utils_throw();
		case 2:
			switch(field::get_rm(modrm))
			{
			case 0: case 1: case 2: case 3: case 5: case 6: case 7: return check(5);
			case 4: return check(6);
			}
			utils_throw();
		case 3:
			return check(1);
		}
		utils_throw();
	}
}
