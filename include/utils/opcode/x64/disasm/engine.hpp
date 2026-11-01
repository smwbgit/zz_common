#pragma once

#include <utils/opcode/x64/disasm/modrm.hpp>

//建议使用反汇编引擎. 此代码不够全面和严谨
namespace utils::opcode::x64::disasm::engine::不建议::simple_engine
{
	namespace inner
	{
		enum class imm_type
		{
			none,
			b,
			w,
			z,
			v,
			O,
		};
	}
	struct s_opcode
	{
		uint64_t size{};	//指令的字节数
		bool is_jmp{};		//是否为直接跳转
		bool is_jcc{};		//是否为条件跳转
		bool is_end{};		//是否为返回指令
		std::vector<uint64_t> jmp_dst;		// is_jmp||is_jcc 时, 跳转目标. 从0到data.size()-1

		struct s_private_data
		{
			struct s_prefix
			{
				struct s_legacy
				{
					bool F3{};
					bool operator_size{};
					bool address_size{};
				}legacy{};
				bool rex_w{};
			}prefix{};
			uint64_t opcode{};
			uint64_t opcode_size{};
			struct s_modrm
			{
				uint8_t mod{};
				uint8_t reg{};
				uint8_t opcode{};
				uint8_t rm{};
				uint8_t all{};
			}modrm{};
			struct s_imm
			{
				uint8_t imm8{};
				uint16_t imm16{};
				uint32_t imm32{};
				uint64_t imm64{};
				uint64_t imm_size{};
			};
			s_imm imm{};
			s_imm imm_2{};

			struct s_read
			{
				bool modrm{};
				bool modrm_M{};	//mod只引用内存
				inner::imm_type imm{inner::imm_type::none};
				inner::imm_type imm_2{inner::imm_type::none};	//第二个立即数  e.g. ENTER指令
			}read{};
		}private_data{};
	};

	namespace inner
	{
		inline std::optional<uint64_t> get_modrm_size(const utils::data & data, uint64_t offset)
		{
			if (offset >= data.size()) return {};
			return utils::opcode::x64::disasm::modrm::get_byte_n(&data.m_data[offset], data.size() - offset);
		}
		template<typename T>
		inline std::optional<T> get_imm(const utils::data & data, uint64_t offset)
		{
			if(offset+sizeof(T) <= data.size())
				return *(T*)&data.m_data[offset];
			else
				return {};
		}
		inline std::optional<uint8_t> get_imm8(const utils::data & data, uint64_t offset)
		{
			return get_imm<uint8_t>(data, offset);
		}
		inline std::optional<uint32_t> get_imm32(const utils::data & data, uint64_t offset)
		{
			return get_imm<uint32_t>(data, offset);
		}
		inline uint64_t 符号扩展_8_to_64(uint8_t value)
		{
			return (uint64_t)(int64_t)(int8_t)value;
		}
		inline uint64_t 符号扩展_32_to_64(uint32_t value)
		{
			return (uint64_t)(int64_t)(int32_t)value;
		}
		struct s_group_opcode
		{
			uint8_t byte{};
			uint8_t mod{};
			uint8_t opcode{};
		};
		inline std::optional<s_group_opcode> get_group_opcode(const utils::data & data, const uint64_t i)
		{
			auto modrm = inner::get_imm8(data, i); if(!modrm.has_value()) return {};

			return s_group_opcode{
				modrm.value(),
				utils::opcode::x64::disasm::modrm::field::get_mod(modrm.value()),
				utils::opcode::x64::disasm::modrm::field::get_opcode(modrm.value())
			};
		}
		inline void append_opcode(s_opcode & opcode, const uint8_t b, uint64_t & i)
		{
			uint64_t bits = 8 * opcode.private_data.opcode_size;
			opcode.private_data.opcode |= ((uint64_t)b<<bits);
			opcode.private_data.opcode_size++;
			i++;
		}
		inline bool group_to_opcode(s_opcode & opcode, const s_group_opcode & group_opcode, uint64_t & i, std::function<bool()> func)
		{
			auto re = func();
			if(re) append_opcode(opcode, group_opcode.byte, i);
			return re;
		}
		namespace read
		{
			inline bool read_prefix(s_opcode & opcode, const utils::data & data, uint64_t & i)
			{
				while(1)
				{
					auto _b = inner::get_imm8(data, i); if(!_b.has_value()) return false;
					const auto b = _b.value();

					//todo 看看以下这些, 是按手册来, 还是实际cpu执行情况来
					//     当 已有rex 时
					//     当 rex 在其他前缀的前面 时

					if(0x40 <= b && b <= 0x4F)
					{
						opcode.private_data.prefix.rex_w = b & 0x08 ? true : false;
					}
					else if(b == 0xF0 || b == 0xF2 || b == 0xF3) //group 1
					{
						opcode.private_data.prefix.legacy.F3 = b == 0xF3;
					}
					else if(b == 0x64 || b == 0x65 || b == 0x26 || b == 0x36 || b == 0x2E || b == 0x3E) //group 2
					{
					}
					else if(b == 0x66) //group 3
					{
						opcode.private_data.prefix.legacy.operator_size = true;
					}
					else if(b == 0x67) //group 4
					{
						opcode.private_data.prefix.legacy.address_size = true;
					}
					else return true;

					i++;
				}
			}
			inline bool read_opcode_second(s_opcode & opcode, const utils::data & data, uint64_t & i)
			{
				//简单检查
				utils_assert(opcode.private_data.opcode == 0x0F);

				auto _b = inner::get_imm8(data, i); if(!_b.has_value()) return false;
				const auto b = _b.value();

				//保存
				append_opcode(opcode, b, i);

				switch(b)
				{
				case 0x00:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0: case 1: case 2: case 3: case 4: case 5: opcode.private_data.read.modrm = true; return true;
					case 6: case 7: return false;
					default: utils_throw();
					}
				}
				case 0x01:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0:
					{
						if(group_opcode->mod == 0x11)
						{
							return group_to_opcode(opcode, *group_opcode, i, [&]()
							{
								switch(group_opcode->byte)
								{
								case 0xC1: case 0xC2: case 0xC3: case 0xC4: return true;
								default: return false;
								}
							});
						}
						else { opcode.private_data.read.modrm = true; return true; }
					}
					case 1:
					{
						if(group_opcode->mod == 0x11)
						{
							return group_to_opcode(opcode, *group_opcode, i, [&]()
							{
								switch(group_opcode->byte)
								{
								case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCF: return true;
								default: return false;
								}
							});
						}
						else { opcode.private_data.read.modrm = true; return true; }
					}
					case 2:
					{
						if(group_opcode->mod == 0x11)
						{
							return group_to_opcode(opcode, *group_opcode, i, [&]()
							{
								switch(group_opcode->byte)
								{
								case 0xD0: case 0xD1: case 0xD4: case 0xD5: case 0xD6: case 0xD7: return true;
								default: return false;
								}
							});
						}
						else { opcode.private_data.read.modrm = true; return true; }
					}
					case 3:
					{
						if(group_opcode->mod == 0x11) return false;
						else { opcode.private_data.read.modrm = true; return true; }
					}
					case 4: opcode.private_data.read.modrm = true; return true;
					case 5: return false;
					case 6: opcode.private_data.read.modrm = true; return true;
					case 7:
					{
						if(group_opcode->mod == 0x11)
						{
							return group_to_opcode(opcode, *group_opcode, i, [&]()
							{
								switch(group_opcode->byte)
								{
								case 0xF8: case 0xF9: return true;
								default: return false;
								}
							});
						}
						else { opcode.private_data.read.modrm = true; return true; }
					}
					default: utils_throw();
					}
				}
				case 0x02: case 0x03: opcode.private_data.read.modrm = true; return true;
				case 0x04: return false;
				case 0x05: case 0x06: case 0x07: case 0x08: case 0x09: return true;
				case 0x0A: return false;
				case 0x0B: return true;
				case 0x0C: return false;
				case 0x0D: return true;
				case 0x0E: case 0x0F: return false;
				case 0x10: case 0x11: opcode.private_data.read.modrm = true; return true;
				case 0x1C: case 0x1D: case 0x1E: return false;
				case 0x1F: opcode.private_data.read.modrm = true; return true;
				case 0x39: return false;
				case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: return false;
				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:  case 0x46:  case 0x47:
				case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D:  case 0x4E:  case 0x4F:
					opcode.private_data.read.modrm = true; return true;
				case 0x62: opcode.private_data.read.modrm = true; return true;
				case 0x70: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x74: case 0x75: case 0x76: opcode.private_data.read.modrm = true; return true;
				case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:  case 0x86:  case 0x87:
				case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D:  case 0x8E:  case 0x8F:
					opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:  case 0x96:  case 0x97:
				case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:  case 0x9E:  case 0x9F:
					opcode.private_data.read.modrm = true; return true;
				case 0xA2: return true;
				case 0xAF: opcode.private_data.read.modrm = true; return true;
				case 0xB1: opcode.private_data.read.modrm = true; return true;
				case 0xB4: case 0xB5: opcode.private_data.read.modrm = true; return true;
				case 0xB6: case 0xB7: opcode.private_data.read.modrm = true; return true;
				case 0xB8:
					if(opcode.private_data.prefix.legacy.F3) {opcode.private_data.read.modrm = true; return true;}
					else return false;
				case 0xBA: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xBE: case 0xBF: opcode.private_data.read.modrm = true; return true;
				case 0xFF: return true;
				default: return false;	//todo 补充其他指令
				//default: utils_throw();
					// 第二个表
				}
			}
			inline bool read_opcode(s_opcode & opcode, const utils::data & data, uint64_t & i)
			{
				auto _b = inner::get_imm8(data, i); if(!_b.has_value()) return false;
				const auto b = _b.value();
				
				//保存
				append_opcode(opcode, b, i);

				switch(b)
				{
				case 0x00: case 0x01: case 0x02: case 0x03: opcode.private_data.read.modrm = true; return true;
				case 0x04: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x05: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x06: case 0x07: return false;
				case 0x08: case 0x09: case 0x0A: case 0x0B: opcode.private_data.read.modrm = true; return true;
				case 0x0C: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x0D: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x0E: return false;
				case 0x0F: return read_opcode_second(opcode, data, i);
				case 0x10: case 0x11: case 0x12: case 0x13: opcode.private_data.read.modrm = true; return true;
				case 0x14: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x15: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x16: case 0x17: return false;
				case 0x18: case 0x19: case 0x1A: case 0x1B: opcode.private_data.read.modrm = true; return true;
				case 0x1C: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x1D: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x1E: case 0x1F: return false;
				case 0x20: case 0x21: case 0x22: case 0x23: opcode.private_data.read.modrm = true; return true;
				case 0x24: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x25: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x26: utils_throw();
				case 0x27: return false;
				case 0x28: case 0x29: case 0x2A: case 0x2B: opcode.private_data.read.modrm = true; return true;
				case 0x2C: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x2D: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x2E: utils_throw();
				case 0x2F: return false;
				case 0x30: case 0x31: case 0x32: case 0x33: opcode.private_data.read.modrm = true; return true;
				case 0x34: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x35: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x36: utils_throw();
				case 0x37: return false;
				case 0x38: case 0x39: case 0x3A: case 0x3B: opcode.private_data.read.modrm = true; return true;
				case 0x3C: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x3D: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x3E: utils_throw();
				case 0x3F: return false;
				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
				case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
					utils_throw();
				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
					return true;
				case 0x60: case 0x61: case 0x62: return false;
				case 0x63: opcode.private_data.read.modrm = true; return true;
				case 0x64: case 0x65: case 0x66: case 0x67: utils_throw();
				case 0x68: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x69: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x6A: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x6B: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x6C: case 0x6D: case 0x6E: case 0x6F: return true;
				case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
				case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
					opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x80: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x81: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0x82: return false;
				case 0x83: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0x84: case 0x85: case 0x86: case 0x87: opcode.private_data.read.modrm = true; return true;
				case 0x88: case 0x89: case 0x8A: case 0x8B: opcode.private_data.read.modrm = true; return true;
				case 0x8C: opcode.private_data.read.modrm = true; return true;
				case 0x8D: opcode.private_data.read.modrm = true; opcode.private_data.read.modrm_M = true; return true;
				case 0x8E: opcode.private_data.read.modrm = true; return true;
				case 0x8F:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0: opcode.private_data.read.modrm = true; return true;
					case 1: case 2: case 3: case 4: case 5: case 6: case 7: return false;
					default: utils_throw();
					}
				}
				case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: return true;
				case 0x98: case 0x99: return true;
				case 0x9A: return false;
				case 0x9B: case 0x9C: case 0x9D: return true;
				case 0x9E: case 0x9F: return true;
				case 0xA0: case 0xA1: case 0xA2: case 0xA3: opcode.private_data.read.imm = inner::imm_type::O; return true;
				case 0xA4: case 0xA5: case 0xA6: case 0xA7: return true;
				case 0xA8: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xA9: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: return true;
				case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
					opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
					opcode.private_data.read.imm = inner::imm_type::v; return true;
				case 0xC0: case 0xC1: opcode.private_data.read.modrm = true; opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xC2: opcode.private_data.read.imm = inner::imm_type::w; return true;
				case 0xC3: return true;
				case 0xC4: case 0xC5: return false;
				case 0xC6: case 0xC7:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0:
						opcode.private_data.read.modrm = true;
						opcode.private_data.read.imm = b == 0xC6 ? inner::imm_type::b : inner::imm_type::z;
						return true;
					case 1: case 2: case 3: case 4: case 5: case 6: return false;
					case 7:
						return group_to_opcode(opcode, *group_opcode, i, [&]()
						{
							switch(group_opcode->byte)
							{
							case 0xF8: opcode.private_data.read.imm = b == 0xC6 ? inner::imm_type::b : inner::imm_type::z; return true;
							default: return false;
							}
						});
					default: utils_throw();
					}
				}
				case 0xC8:
					opcode.private_data.read.imm = inner::imm_type::w;
					opcode.private_data.read.imm_2 = inner::imm_type::b;
					return true;
				case 0xC9: return true;
				case 0xCA: opcode.private_data.read.imm = inner::imm_type::w; return true;
				case 0xCB: return true;
				case 0xCC: return true;
				case 0xCD: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xCE: return false;
				case 0xCF: return true;
				case 0xD0: case 0xD1: case 0xD2: case 0xD3: opcode.private_data.read.modrm = true; return true;
				case 0xD4: case 0xD5: case 0xD6: return false;
				case 0xD7: return true;
				case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
					opcode.private_data.read.modrm = true; return true;
				case 0xE0: case 0xE1: case 0xE2: case 0xE3: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xE4: case 0xE5: case 0xE6: case 0xE7: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xE8: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0xE9: opcode.private_data.read.imm = inner::imm_type::z; return true;
				case 0xEA: return false;
				case 0xEB: opcode.private_data.read.imm = inner::imm_type::b; return true;
				case 0xEC: case 0xED: case 0xEE: case 0xEF: return true;
				case 0xF0: utils_throw();
				case 0xF1: return true;
				case 0xF2: case 0xF3: utils_throw();
				case 0xF4: case 0xF5: return true;
				case 0xF6: case 0xF7:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0:
						opcode.private_data.read.modrm = true;
						opcode.private_data.read.imm = b == 0xF6 ? inner::imm_type::b : inner::imm_type::z; 
						return true;
					case 1: return false;
					case 2: case 3: case 4: case 5: case 6: case 7: opcode.private_data.read.modrm = true; return true;
					default: utils_throw();
					}
				}
				case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: return true;
				case 0xFE:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0: case 1: opcode.private_data.read.modrm = true; return true;
					case 2: case 3: case 4: case 5: case 6: case 7: return false;
					default: utils_throw();
					}
				}
				case 0xFF:
				{
					const auto group_opcode = inner::get_group_opcode(data, i); if(!group_opcode.has_value()) return false;
					switch(group_opcode->opcode)
					{
					case 0: case 1: case 2: case 3: case 4: case 5: case 6: opcode.private_data.read.modrm = true; return true;
					case 7: return false;
					default: utils_throw();
					}
				}
				default: utils_throw();
				}
			}
			inline bool read_modrm(s_opcode & opcode, const utils::data & data, uint64_t & i)
			{
				if(!opcode.private_data.read.modrm) return true;
				
				auto modrm_size = inner::get_modrm_size(data, i);
				if(!modrm_size.has_value()) return false;
				
				//保存
				namespace field = utils::opcode::x64::disasm::modrm::field;
				const auto b = data[i];
				opcode.private_data.modrm.all = b;
				opcode.private_data.modrm.mod = field::get_mod(b);
				opcode.private_data.modrm.reg = field::get_reg(b);
				opcode.private_data.modrm.opcode = field::get_opcode(b);
				opcode.private_data.modrm.rm = field::get_rm(b);

				//简单检查
				if(opcode.private_data.read.modrm_M)
				{
					utils_assert(opcode.private_data.modrm.mod != 0x11);
				}

				i += modrm_size.value();
				return true;
			}
			inline bool read_imm(s_opcode & opcode, const utils::data & data, uint64_t & i)
			{
				auto read_one_imm = [&](decltype(opcode.private_data.read.imm) read_request,
					decltype(opcode.private_data.imm) & _imm)
				{
					auto read_and_save = [&](uint64_t size)
					{
						switch(size)
						{
						case 8:
						{
							auto imm = inner::get_imm<uint64_t>(data, i); if(!imm.has_value()) return false;
							_imm.imm64 = imm.value();
							break;
						}
						case 4:
						{
							auto imm = inner::get_imm<uint32_t>(data, i); if(!imm.has_value()) return false;
							_imm.imm32 = imm.value();
							break;
						}
						case 2:
						{
							auto imm = inner::get_imm<uint16_t>(data, i); if(!imm.has_value()) return false;
							_imm.imm16 = imm.value();
							break;
						}
						case 1:
						{
							auto imm = inner::get_imm<uint8_t>(data, i); if(!imm.has_value()) return false;
							_imm.imm8 = imm.value();
							break;
						}
						default: utils_throw();
						}
						_imm.imm_size = size;
						i += size;
						return true;
					};

					using imm_type = inner::imm_type;
					switch(read_request)
					{
					case imm_type::none: return true;
					case imm_type::b: return read_and_save(1);
					case imm_type::w: return read_and_save(2);
					case imm_type::z:
						if(opcode.private_data.prefix.legacy.operator_size) return read_and_save(2);
						else return read_and_save(4);
					case imm_type::v:
						//简单检查
						utils_assert(0xB8 <= opcode.private_data.opcode && opcode.private_data.opcode <= 0xBF);

						if(opcode.private_data.prefix.rex_w)
							return read_and_save(8);
						else if(opcode.private_data.prefix.legacy.operator_size)
							return read_and_save(2);
						else
							return read_and_save(4);
					case imm_type::O:
						if(opcode.private_data.prefix.legacy.address_size)
							return read_and_save(4);
						else
							return read_and_save(8);
					}
					utils_throw();
				};
				
				return 
					read_one_imm(opcode.private_data.read.imm,   opcode.private_data.imm)  &&
					read_one_imm(opcode.private_data.read.imm_2, opcode.private_data.imm_2);
			}
		}
	}

	//从data[offset]开始, 读取一条指令
	inline std::optional<s_opcode> get_one_opcode(const utils::data & data, uint64_t offset, uint64_t base_address)
	{
		utils_assert(offset < data.size());

		uint64_t i = offset;
		s_opcode opcode;
		if(!inner::read::read_prefix(opcode, data, i)) return {};
		if(!inner::read::read_opcode(opcode, data, i)) return {};
		if(!inner::read::read_modrm(opcode, data, i)) return {};
		if(!inner::read::read_imm(opcode, data, i)) return {};

		opcode.size = i-offset;
		switch(opcode.private_data.opcode)
		{
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:	//jcc
		case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:	//jcc
		case 0xE0: case 0xE1: case 0xE2: case 0xE3: //loop或jrcxz
			opcode.is_jcc = true;
			opcode.jmp_dst.push_back(offset + opcode.size);
			opcode.jmp_dst.push_back(offset + opcode.size +
				inner::符号扩展_8_to_64(opcode.private_data.imm.imm8));
			break;
		case 0xC3:	//ret
			opcode.is_end = true;
			break;
		case 0xCA: case 0xCB: //far ret
			opcode.is_end = true;
			break;
		case 0xCC:	//int3
			opcode.is_end = true;
			break;
		case 0xCD:	//int
			switch(opcode.private_data.imm.imm8)
			{
				//未查到: 不知道是普通函数还是不会返回的那种
			case 0xA6: break;
			default: break;

				//已查到
			case 0x29: opcode.is_end = true; break;
			}
			break;
		case 0xE9:	//jmp
			if(opcode.private_data.imm.imm_size != 4) return {};	//手册上说不支持16位的
			opcode.is_jmp = true;
			opcode.jmp_dst.push_back(offset + opcode.size +
				inner::符号扩展_32_to_64(opcode.private_data.imm.imm32));
			break;
		case 0xEB: 	//jmp
			opcode.is_jmp = true;
			opcode.jmp_dst.push_back(offset + opcode.size +
				inner::符号扩展_8_to_64(opcode.private_data.imm.imm8));
			break;
		case 0xFF:
			switch(opcode.private_data.modrm.opcode)
			{
			case 4: opcode.is_end = true; break;	// jmp 间接
			case 5: opcode.is_end = true; break;	// far jmp 间接
			}
			break;
		case 0x800F: case 0x810F: case 0x820F: case 0x830F: case 0x840F: case 0x850F:  case 0x860F:  case 0x870F:	//jcc
		case 0x880F: case 0x890F: case 0x8A0F: case 0x8B0F: case 0x8C0F: case 0x8D0F:  case 0x8E0F:  case 0x8F0F:
			utils_assert(opcode.private_data.imm.imm_size == 4);	//手册上说不支持16位的
			opcode.is_jcc = true;
			opcode.jmp_dst.push_back(offset + opcode.size);
			opcode.jmp_dst.push_back(offset + opcode.size +
				inner::符号扩展_32_to_64(opcode.private_data.imm.imm32));
			break;
		case 0xFF0F:	//ud
			opcode.is_end = true;
			break;
		}

		return opcode;
	}
}