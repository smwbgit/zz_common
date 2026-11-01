#pragma once

#include <utils/opcode/dalvik/common/instruction.hpp>
#include <utils/basic/bits.hpp>
#include <utils/queue.hpp>

namespace utils::opcode::dalvik::decode
{
	struct s_switch_packed_table
	{
		u8 index;
		s8 first_value;
		std::vector<u8> targets;	//已计算过当前地址
	};
	struct s_switch_sparse_table
	{
		u8 index;
		struct s_entry
		{
			s8 value;
			u8 target;	//已计算过当前地址
		};
		std::vector<s_entry> entries;
	};
	struct s_array_fill_table
	{
		u8 index;
		u8 element_width;
		std::vector<s8> elements;
	};
	struct s_operands
	{
		std::vector<u8> registers;
		std::optional<u8> const_pool_index;
		std::optional<s8> signed_imm;
		std::optional<s8> signed_branch;
		std::optional<u8> register_first;
		std::optional<u8> register_count;
		std::optional<u8> switch_sparse_table;
		std::optional<u8> switch_packed_table;
		std::optional<u8> array_fill_table;
	};
	struct s_instruction
	{
		u8 index;
		std::vector<u8> froms;
		std::vector<u8> tos;
		const utils::opcode::dalvik::common::instruction::s_opcode_desc* opcode_desc;
		s_operands operands;
		std::u8string text;
	};
	struct s_function_code
	{
		std::vector<u1> is_used;	//0:未使用 1:指令的首个单元 2:指令的剩余单元 3:紧密switch的跳转表 4:稀疏switch的跳转表
		std::map<u8, s_instruction> instructions;
		std::map<u8, s_switch_packed_table> switch_packed_tables;
		std::map<u8, s_switch_sparse_table> switch_sparse_tables;
		std::map<u8, s_array_fill_table> array_fill_tables;
	};
	namespace inner::decode_function_code
	{
		class RealImpl
		{
		private:
			const std::vector<u2> & m_code_data;
			s_function_code m_function_code{};
			std::function<std::vector<u8>()> m_get_catch_handlers;
		public:
			RealImpl(const std::vector<u2> & code_data, std::function<std::vector<u8>()> get_catch_handlers)
				: m_code_data{code_data}
				, m_get_catch_handlers{get_catch_handlers}
			{
			}
			s_function_code execute()
			{
				//初始化：全部标记为未使用
				m_function_code.is_used.resize(m_code_data.size());
		
				//起始遍历位置
				std::queue<u8> nexts;
				nexts.push(0);//第一条指令
				utils::queue(nexts).push_many(m_get_catch_handlers());//catch处理块的第一条指令

				while(!nexts.empty())
				{
					//获取一个待查看指令的指令位置
					const auto i = utils::queue(nexts).pop();
			
					//已经查看过
					if(m_function_code.is_used[i] == 1) continue;
					utils_assert(m_function_code.is_used[i] == 0);

					//读取指令
					auto instruction = decode_instruction(i);
					
					//设置待查看的指令
					utils::queue(nexts).push_many(instruction.tos);
				
					//保存
					utils::map(m_function_code.instructions).insert_not_exist(
						instruction.index, std::move(instruction));
				}
				return m_function_code;
			}
		private:
			void mark_used(u8 i, u8 size, u1 first, u1 others)
			{
				for(u8 j : utils::iterator_index(size))
				{
					//简单检查
					utils_assert(m_function_code.is_used[i+j] == 0);

					m_function_code.is_used[i+j] = (j == 0 ? first : others);
				}
			}
			s_instruction decode_instruction(u8 i)
			{
				//读取指令
				s_instruction instruction{};
				{
					instruction.index = i;
					decode_opcode(instruction);
					decode_operands(instruction);
					decode_tos(instruction);
					decode_text(instruction);
				}

				//更新状态
				mark_used(i, instruction.opcode_desc->format.size, 1, 2);

				//给出
				return std::move(instruction);
			}
			void decode_opcode(s_instruction & instruction)
			{
				//简单检查
				utils_assert(instruction.index < m_code_data.size());

				u1 opcode = utils::basic::bits::get_low_half(m_code_data[instruction.index]);
				instruction.opcode_desc = &utils::opcode::dalvik::common::instruction::get_opcode_desc(opcode);
			}
#pragma region 辅助函数
			template<u8 start_bit, u8 bit_n>
			u8 decode_operand_unsigned(s_instruction & instruction)
			{
				if (start_bit == 8 && bit_n == 8 ||
					start_bit == 8 && bit_n == 4 ||
					start_bit == 12 && bit_n == 4)
				{
					return utils::basic::bits::get_bits(m_code_data[instruction.index],
						start_bit, bit_n);
				}
				if (start_bit == 16 && bit_n == 16 ||
					start_bit == 16 && bit_n == 8 ||
					start_bit == 24 && bit_n == 8)
				{
					return utils::basic::bits::get_bits(m_code_data[instruction.index+1],
						start_bit-16, bit_n);
				}
				if (start_bit == 32+0 && bit_n == 4 ||
					start_bit == 32+4 && bit_n == 4 ||
					start_bit == 32+8 && bit_n == 4 ||
					start_bit == 32+12 && bit_n == 4 ||
					start_bit == 32+0 && bit_n == 16)
				{
					return utils::basic::bits::get_bits(m_code_data[instruction.index+2],
						start_bit-32, bit_n);
				}
				if (start_bit == 16 && bit_n == 32)
					return *(u4*)&m_code_data[instruction.index+1];
				if (start_bit == 16 && bit_n == 64)
					return *(u8*)&m_code_data[instruction.index+1];

				utils_throw();
			}
			template<u8 start_bit, u8 bit_n>
			s8 decode_operand_signed(s_instruction & instruction)
			{
				u8 value = decode_operand_unsigned<start_bit,bit_n>(instruction);
				return utils::basic::bits::signed_extend(value, bit_n);
			}
			template<u8 start_bit, u8 bit_n>
			void decode_operand_register(s_instruction & instruction)
			{
				instruction.operands.registers.push_back(
					decode_operand_unsigned<start_bit,bit_n>(instruction));
			}
			template<u8 start_bit, u8 bit_n>
			void decode_operand_padding(s_instruction & instruction)
			{
				utils_assert((0 == decode_operand_unsigned<start_bit,bit_n>(instruction)));
			}
			template<u8 start_bit, u8 bit_n>
			void decode_operand_branch(s_instruction & instruction)
			{
				instruction.operands.signed_branch = decode_operand_signed<start_bit,bit_n>(instruction);
			}
			template<u8 start_bit, u8 bit_n>
			void decode_operand_imm(s_instruction & instruction)
			{
				instruction.operands.signed_imm = decode_operand_signed<start_bit,bit_n>(instruction);
			}
			template<u8 start_bit, u8 bit_n>
			void decode_operand_pool_index(s_instruction & instruction)
			{
				instruction.operands.const_pool_index = decode_operand_unsigned<start_bit,bit_n>(instruction);
			}
#pragma endregion
			void decode_operands(s_instruction & instruction)
			{
				//准备数据
				const auto & format = instruction.opcode_desc->format;
				const auto i = instruction.index;
				using Format = utils::opcode::dalvik::common::instruction::Format;
				auto & operands = instruction.operands;

				//简单检查
				utils_assert(i+format.size <= m_code_data.size());

				switch(format.format)
				{
				case Format::_10x:
				{
					break;
				}
				case Format::_10t:
				{
					decode_operand_branch<8,8>(instruction);
					break;
				}
				case Format::_11n:
				{
					decode_operand_register<12,4>(instruction);
					decode_operand_imm<8,4>(instruction);
					break;
				}
				case Format::_11x:
				{
					decode_operand_register<8,8>(instruction);
					break;
				}
				case Format::_12x:
				{
					decode_operand_register<12,4>(instruction);
					decode_operand_register<8,4>(instruction);
					break;
				}
				case Format::_20t:
				{
					decode_operand_padding<8,8>(instruction);
					decode_operand_branch<16,16>(instruction);
					break;
				}
				case Format::_21c:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_pool_index<16,16>(instruction);
					break;
				}
				case Format::_21h:
				{
					decode_operand_register<8,8>(instruction);
					auto value = (u4)decode_operand_unsigned<16,16>(instruction);
					operands.signed_imm = (s4)(value << 16);
					break;
				}
				case Format::_21s:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_imm<16,16>(instruction);
					break;
				}
				case Format::_21t:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_branch<16,16>(instruction);
					break;
				}
				case Format::_22b:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_register<16,8>(instruction);
					decode_operand_imm<24,8>(instruction);
					break;
				}
				case Format::_22c:
				{
					decode_operand_register<12,4>(instruction);
					decode_operand_register<8,4>(instruction);
					decode_operand_pool_index<16,16>(instruction);
					break;
				}
				case Format::_22s:
				{
					decode_operand_register<12,4>(instruction);
					decode_operand_register<8,4>(instruction);
					decode_operand_imm<16,16>(instruction);
					break;
				}
				case Format::_22t:
				{
					decode_operand_register<12,4>(instruction);
					decode_operand_register<8,4>(instruction);
					decode_operand_branch<16,16>(instruction);
					break;
				}
				case Format::_22x:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_register<16,16>(instruction);
					break;
				}
				case Format::_23x:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_register<16,8>(instruction);
					decode_operand_register<24,8>(instruction);
					break;
				}
				case Format::_30t:
				{
					decode_operand_padding<8,8>(instruction);
					decode_operand_branch<16,32>(instruction);
					break;
				}
				case Format::_31i:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_imm<16,32>(instruction);
					break;
				}
				case Format::_31t:
				{
					decode_operand_register<8,8>(instruction);
					decode_operand_branch<16,32>(instruction);

					//读取附加数据
					if (instruction.opcode_desc->opcode == 0x2c)	// sparse-switch指令
						decode_switch_sparse_table(instruction);
					else if (instruction.opcode_desc->opcode == 0x2b)	// packed-switch指令
						decode_switch_packed_table(instruction);
					else if (instruction.opcode_desc->opcode == 0x26)	// fill-array-data指令
						decode_array_fill_table(instruction);
					else
						utils_throw();
					break;
				}
				case Format::_35c:
				{
					const auto reg_n = decode_operand_unsigned<12,4>(instruction);
					utils_assert(reg_n>=0 && reg_n<=5);
					if(reg_n>=1) decode_operand_register<32+0,4>(instruction);
					if(reg_n>=2) decode_operand_register<32+4,4>(instruction);
					if(reg_n>=3) decode_operand_register<32+8,4>(instruction);
					if(reg_n>=4) decode_operand_register<32+12,4>(instruction);
					if(reg_n>=5) decode_operand_register<8,4>(instruction);

					decode_operand_pool_index<16,16>(instruction);

					//简单检查
					if(reg_n<1) decode_operand_padding<32+0,4>(instruction);
					if(reg_n<2) decode_operand_padding<32+4,4>(instruction);
					if(reg_n<3) decode_operand_padding<32+8,4>(instruction);
					if(reg_n<4) decode_operand_padding<32+12,4>(instruction);
					if(reg_n<5) decode_operand_padding<8,4>(instruction);

					break;
				}
				case Format::_3rc:
				{
					operands.register_first = decode_operand_unsigned<32,16>(instruction);
					operands.register_count = decode_operand_unsigned<8,8>(instruction);
					decode_operand_pool_index<16,16>(instruction);
					break;
				}
				case Format::_51l:
				{
					//temp
					if(instruction.index == 0x7c)
					{
						int i = 0; i = i;
					}

					decode_operand_register<8,8>(instruction);
					decode_operand_imm<16,64>(instruction);
					break;
				}
				default:
					utils_throw();
				}
			}
			//获得 指令地址和数据地址
			u8 get_index(s_instruction & instruction, s8 offset)
			{
				u8 index = utils::basic::store_in<u8>((s8)instruction.index + offset);
				utils_assert(index < m_code_data.size());
				return index;
			}
			//特化
			u8 get_index(s_instruction & instruction, u8 offset)
			{
				u8 index = instruction.index + offset;
				utils_assert(index < m_code_data.size());
				return index;
			}
			bool is_not_used(u8 i, u8 size)
			{
				for(auto j : utils::iterator_index(size))
					if(m_function_code.is_used[i+j])
						return false;
				return true;
			}
			void decode_switch_packed_table(s_instruction & instruction)
			{
				//数据所在位置
				const auto i = get_index(instruction, instruction.operands.signed_branch.value());
				instruction.operands.switch_packed_table = i;
				s_switch_packed_table switch_packed_table{};
				switch_packed_table.index = i;

				//简单检查
				utils_assert(is_not_used(i, 4));
				utils_assert(m_code_data[i] == 0x0100);

				u2 size = m_code_data[i+1];
				switch_packed_table.first_value = *(s4*)&m_code_data[i+2];

				//简单检查
				utils_assert(is_not_used(i+4, 2*size));

				for(auto j : utils::iterator_index(size))
				{
					s8 offset = *(s4*)&m_code_data[i+4+j*2];
					switch_packed_table.targets.push_back(get_index(instruction, offset));
				}

				//更新状态
				mark_used(i,4+2*size, 10, 11);
				
				//保存
				utils::map(m_function_code.switch_packed_tables).insert_not_exist(
					i, std::move(switch_packed_table));
			};
			void decode_switch_sparse_table(s_instruction & instruction)
			{
				//数据所在位置
				const auto i = get_index(instruction, instruction.operands.signed_branch.value());
				instruction.operands.switch_sparse_table = i;
				s_switch_sparse_table switch_sparse_table{};
				switch_sparse_table.index = i;

				//简单检查
				utils_assert(is_not_used(i, 2));
				utils_assert(m_code_data[i] == 0x0200);
				
				//表格大小
				u2 size = m_code_data[i+1];

				//简单检查
				utils_assert(is_not_used(i+2, 4*size));
				
				//读取每个跳转项
				switch_sparse_table.entries.resize(size);
				for(auto j : utils::iterator_index(size))
				{
					switch_sparse_table.entries[j].value = *(s4*)&m_code_data[i+2+j*2];
				}
				for(auto j : utils::iterator_index(size))
				{
					s8 offset = *(s4*)&m_code_data[i+2+size*2+j*2];
					switch_sparse_table.entries[j].target = get_index(instruction, offset);
				}

				//更新状态
				mark_used(i, 2+4*size, 20, 21);

				//保存
				utils::map(m_function_code.switch_sparse_tables).insert_not_exist(
					i, std::move(switch_sparse_table));
			}
			void decode_array_fill_table(s_instruction & instruction)
			{
				//数据所在位置
				const auto i = get_index(instruction, instruction.operands.signed_branch.value());
				instruction.operands.array_fill_table = i;
				s_array_fill_table array_fill_table{};
				array_fill_table.index = i;

				//简单检查
				utils_assert(is_not_used(i, 4));
				utils_assert(m_code_data[i] == 0x0300);

				array_fill_table.element_width = m_code_data[i+1];
				u8 size = *(u4*)&m_code_data[i+2];
			
				//简单检查
				utils_assert(
					array_fill_table.element_width == 1 ||
					array_fill_table.element_width == 2 ||
					array_fill_table.element_width == 4 ||
					array_fill_table.element_width == 8);
				utils_assert(size > 0);

				//数据块大小，按u2计算
				u8 block_size{};
				if(array_fill_table.element_width == 1)
				{
					block_size = (size+1)/2;	//如果个数是单数，则按双数算
				}
				else
				{
					block_size = size * array_fill_table.element_width / 2;
				}

				//简单检查
				utils_assert(is_not_used(i+4, block_size));

				//简单检查: padding
				if(array_fill_table.element_width == 1 && size % 2 == 1)
				{
					const auto v = m_code_data[i+4+block_size-1];
					utils_assert(0 == utils::basic::bits::get_high_half(v));	//应该是padding 0
				}
				
				const auto p = (const u1*)&m_code_data[i+4];
				for(auto j : utils::iterator_index(size))
				{
					s8 value{};
					switch(array_fill_table.element_width)
					{
					case 1: value = *(s1*)(p+j*1); break;
					case 2: value = *(s2*)(p+j*2); break;
					case 4: value = *(s4*)(p+j*4); break;
					case 8: value = *(s8*)(p+j*8); break;
					default: utils_throw();
					}

					array_fill_table.elements.push_back(value);
				}

				//更新状态
				mark_used(i, 4+block_size, 30, 31);
				
				//保存
				utils::map(m_function_code.array_fill_tables).insert_not_exist(
					i, std::move(array_fill_table));
			};
			void decode_tos(s_instruction & instruction)
			{
				//准备数据
				auto & tos = instruction.tos;
				const auto & opcode_desc = *instruction.opcode_desc;

				if(opcode_desc.flags.is_continue)
					tos.push_back(get_index(instruction, opcode_desc.format.size));
				if(opcode_desc.flags.is_branch)
					tos.push_back(get_index(instruction, instruction.operands.signed_branch.value()));
				if(opcode_desc.flags.is_switch)
				{
					if(opcode_desc.opcode == 0x2c)	//sparse-switch
					{
						//准备数据
						u8 table_index = instruction.operands.switch_sparse_table.value();
						const auto & table = m_function_code.switch_sparse_tables.at(table_index);

						for(const auto & one : table.entries)
							tos.push_back(one.target);
					}
					else if(opcode_desc.opcode == 0x2b)	//packed-switch
					{
						//准备数据
						u8 table_index = instruction.operands.switch_packed_table.value();
						const auto & table = m_function_code.switch_packed_tables.at(table_index);

						for(const auto & one : table.targets)
							tos.push_back(one);
					}
					else
						utils_throw();
				}

				//简单检查
				for(const auto to : tos)
					utils_assert(to < m_code_data.size());
			}
			void decode_text(s_instruction & instruction)
			{
				instruction.text = instruction.opcode_desc->name;
			}
		};
	}
	inline s_function_code decode_function_code(const std::vector<u2> & code_data, std::function<std::vector<u8>()> get_catch_handlers)
	{
		//转发
		return inner::decode_function_code::RealImpl{code_data, get_catch_handlers}.execute();
	}
	namespace inner::dump_function_code
	{
		class RealImpl
		{
		private:
			using PoolType = utils::opcode::dalvik::common::instruction::PoolType;
		private:
			const s_function_code & m_function_code;
			const u8 m_tab_n{};
			const std::vector<u2> & m_code_data;
			std::function<std::u8string(PoolType pool_type, u8 index)> m_get_text_for_const_pool;
			std::u8string m_text;
		public:
			RealImpl(const s_function_code & function_code, u8 tab_n, const std::vector<u2> & code_data, std::function<std::u8string(PoolType pool_type, u8 index)> get_text_for_const_pool)
				: m_function_code{function_code}
				, m_tab_n{tab_n}
				, m_code_data{code_data}
				, m_get_text_for_const_pool{get_text_for_const_pool}
			{
			}
			std::u8string execute()
			{
				u8 i{};
				while(1)
				{
					if (i == m_code_data.size()) break;

					//简单检查
					utils_assert(i < m_code_data.size());
					
					////temp
					//{
					//	if(m_function_code.is_used[i] == 30 || m_function_code.is_used[i] == 31)
					//	{
					//		i++; continue;
					//	}
					//}

					switch(m_function_code.is_used[i])
					{
					case 0:
						dump_unused(i);
						break;
					case 1:
						dump_instruction(i);
						break;
					case 10:
						dump_switch_packed_table(i);
						break;
					case 20:
						dump_switch_sparse_table(i);
						break;
					case 30:
						dump_array_fill_table(i);
						break;
					default:
						utils_throw();
					}
				}
				return m_text;
			}
		private:
			void dump_unused(u8 & i)
			{
				//简单检查
				utils_assert(m_function_code.is_used[i] == 0);
				utils_assert(m_code_data[i] == 0);

				//dump
				m_text += utils::u8string(u8"\t").repeat(m_tab_n);
				dump_hex_data(i, 1);
				m_text += utils::text::format::format(u8"padding");
				m_text += u8"\n";

				//更新状态
				i += 1;
			}
			void dump_hex_data(u8 i, u8 size)
			{
				//简单检查
				utils_assert(size > 0);

				m_text += utils::text::format::format(u8"0x%02llx ", i);
				m_text += utils::text::hex::to_text(
					utils::data_view
					{
						(const u1*)&m_code_data[i],
						size*2,
					},
					u8" ",
					false);
				if (size < 3)
					m_text += utils::u8string(u8"      ").repeat(3-size);
				m_text += u8" ";
			}
			void dump_tab(u8 more_tab_n = 0)
			{
				m_text += utils::u8string(u8"\t").repeat(m_tab_n + more_tab_n);
			}
			void dump_array_fill_table(u8 & i)
			{
				//简单检查
				utils_assert(m_function_code.is_used[i] == 30);

				//准备数据
				const auto & array_fill_table = utils::map(m_function_code.array_fill_tables)
					.get_existed_value(i);
				const auto element_width = array_fill_table.element_width;
				const auto & elements = array_fill_table.elements;
				
				dump_tab();
				dump_hex_data(i, 4);
				m_text += utils::text::format::format(u8"data: array_fill_data element_width=%lld element_n=%lld", element_width, elements.size());
				m_text += u8"\n";
				if(element_width >= 2)
				{
					//简单检查
					utils_assert(
						element_width == 2 ||
						element_width == 4 ||
						element_width == 8);

					for(const auto value : utils::iterator_and_give_me_i(elements))
					{
						dump_tab();
						dump_hex_data(i+4+value.i*element_width/2, element_width/2);
						m_text += utils::text::format::format(u8"data: array[%lld]=%lld", value.i, value.value);
						m_text += u8"\n";
					}

					//更新状态
					i += 4 + elements.size() * element_width/2;
				}
				else
				{
					//简单检查
					utils_assert(element_width == 1);

					const auto line_n = (elements.size()+1)/2;
					const auto element_n = elements.size();
					for(u8 j = 0; j < line_n; j++)
					{
						dump_tab();
						dump_hex_data(i+4+j, 1);
						m_text += utils::text::format::format(u8"data: [0x%llx] %lld", j*2, elements[j*2]);
						if(j*2 + 1 < element_n)
						{
							m_text += utils::text::format::format(u8"\t[0x%llx] %lld", j*2+1, elements[j*2+1]);
						}
						m_text += u8"\n";
					}

					//更新状态
					i += 4 + line_n;
				}

			}
			void dump_switch_packed_table(u8 & i)
			{
				//简单检查
				utils_assert(m_function_code.is_used[i] == 10);

				//准备数据
				const auto & switch_packed_table = utils::map(m_function_code.switch_packed_tables)
					.get_existed_value(i);
				
				m_text += utils::u8string(u8"\t").repeat(m_tab_n);
				dump_hex_data(switch_packed_table.index, 4);
				m_text += utils::text::format::format(u8"data: packed_switch first_value=%lld", switch_packed_table.first_value);
				m_text += u8"\n";
				for(const auto & target : utils::iterator_and_give_me_i(switch_packed_table.targets))
				{
					m_text += utils::u8string(u8"\t").repeat(m_tab_n);
					dump_hex_data(switch_packed_table.index+4+target.i*2, 2);
					m_text += utils::text::format::format(u8"data: case %lld: goto 0x%llx (绝对)", switch_packed_table.first_value + target.i, target.value);
					m_text += u8"\n";
				}

				//更新状态
				i += 4 + switch_packed_table.targets.size() * 2;
			}
			void dump_switch_sparse_table(u8 & i)
			{
				//简单检查
				utils_assert(m_function_code.is_used[i] == 20);

				//准备数据
				const auto & switch_sparse_table = utils::map(m_function_code.switch_sparse_tables)
					.get_existed_value(i);
				
				m_text += utils::u8string(u8"\t").repeat(m_tab_n);
				dump_hex_data(switch_sparse_table.index, 2);
				m_text += utils::text::format::format(u8"data: sparse_switch");
				m_text += u8"\n";
				for(const auto & entry : utils::iterator_and_give_me_i(switch_sparse_table.entries))
				{
					m_text += utils::u8string(u8"\t").repeat(m_tab_n);
					dump_hex_data(switch_sparse_table.index+2+entry.i*2, 2);
					m_text += utils::text::format::format(u8"data: case %lld //goto 0x%llx 绝对地址", entry.value.value, entry.value.target);
					m_text += u8"\n";
				}
				for(const auto & entry : utils::iterator_and_give_me_i(switch_sparse_table.entries))
				{
					m_text += utils::u8string(u8"\t").repeat(m_tab_n);
					dump_hex_data(switch_sparse_table.index+2+switch_sparse_table.entries.size()*2+entry.i*2, 2);
					m_text += utils::text::format::format(u8"data: goto 0x%llx 绝对地址 //case %lld", entry.value.target, entry.value.value);
					m_text += u8"\n";
				}

				//更新状态
				i += 2 + switch_sparse_table.entries.size() * 4;
			}
			//获得 指令地址和数据地址
			u8 get_index(const s_instruction & instruction, s8 offset)
			{
				u8 index = utils::basic::store_in<u8>((s8)instruction.index + offset);
				utils_assert(index < m_code_data.size());
				return index;
			}
#pragma region 辅助函数
			void dump_operand_branch(const s_instruction & instruction)
			{
				const auto index = get_index(instruction, instruction.operands.signed_branch.value());
				m_text += utils::text::format::format(u8"0x%llx.abs", index);
			}
			void dump_operand_switch_packed_table(const s_instruction & instruction)
			{
				const auto table_index = instruction.operands.switch_packed_table.value();
				const auto & table = utils::map(m_function_code.switch_packed_tables).get_existed_value(table_index);
				for(const auto j : utils::iterator_index(table.targets.size()))
				{
					dump_tab();
					m_text += utils::u8string(u8" ").repeat(27);
					m_text += utils::text::format::format(u8"case %lld: goto 0x%llx.abs", table.first_value+j, table.targets[j]);
					m_text += u8"\n";
				}
				m_text.erase(m_text.end()-1);
			}
			void dump_operand_switch_sparse_table(const s_instruction & instruction)
			{
				const auto table_index = instruction.operands.switch_sparse_table.value();
				const auto & table = utils::map(m_function_code.switch_sparse_tables).get_existed_value(table_index);
				for(const auto & entry : table.entries)
				{
					dump_tab();
					m_text += utils::u8string(u8" ").repeat(27);
					m_text += utils::text::format::format(u8"case %lld: goto 0x%llx.abs", entry.value, entry.target);
					m_text += u8"\n";
				}
				m_text.erase(m_text.end()-1);
			}
			void dump_operand_array_fill_table(const s_instruction & instruction)
			{
				const auto table_index = instruction.operands.array_fill_table.value();
				const auto & table = utils::map(m_function_code.array_fill_tables).get_existed_value(table_index);
				for(const auto & element : utils::iterator_and_give_me_i(table.elements))
				{
					dump_tab();
					m_text += utils::u8string(u8" ").repeat(27);
					m_text += utils::text::format::format(u8"array[%lld]=%lld", element.i, element.value);
					m_text += u8"\n";
				}
				m_text.erase(m_text.end()-1);
			}
			void dump_operand_register(const s_instruction & instruction, u8 index_in_operand)
			{
				const auto index = instruction.operands.registers.at(index_in_operand);
				m_text += utils::text::format::format(u8"r%lld", index);
			}
			void dump_operand_imm(const s_instruction & instruction)
			{
				const auto value = instruction.operands.signed_imm.value();
				m_text += utils::text::format::format(u8"%lld", value);
			}
			void dump_comma()
			{
				m_text += u8", ";
			}
			void dump_operand_pool_index(const s_instruction & instruction)
			{
				//准备数据
				using PoolType = utils::opcode::dalvik::common::instruction::PoolType;
				const auto pool_type = instruction.opcode_desc->pool_type;
				const auto pool_index = instruction.operands.const_pool_index.value();

				switch(pool_type)
				{
				case PoolType::String:
					m_text += u8"string_pool";
					break;
				case PoolType::Type:
					m_text += u8"type_pool";
					break;
				case PoolType::Field:
					m_text += u8"field_pool";
					break;
				case PoolType::Method:
					m_text += u8"method_pool";
					break;
				default:
					utils_throw();
				}

				m_text += utils::text::format::format(u8"[0x%llx]", pool_index);
				m_text += u8"=";
				m_text += m_get_text_for_const_pool(pool_type, pool_index);
			}
#pragma endregion
			void dump_operands(const s_instruction & instruction)
			{
				//准备数据
				using Format = utils::opcode::dalvik::common::instruction::Format;

				switch(instruction.opcode_desc->format.format)
				{
				case Format::_10x:
					break;
					
				case Format::_10t:
				{
					dump_operand_branch(instruction);
					break;
				}
				case Format::_11n:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_11x:
				{
					dump_operand_register(instruction, 0);
					break;
				}
				case Format::_12x:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					break;
				}
				case Format::_20t:
				{
					dump_operand_branch(instruction);
					break;
				}
				case Format::_21c:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_pool_index(instruction);
					break;
				}
				case Format::_21h:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_21s:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_21t:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_branch(instruction);
					break;
				}
				case Format::_22b:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_22c:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					dump_comma();
					dump_operand_pool_index(instruction);
					break;
				}
				case Format::_22s:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_22t:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					dump_comma();
					dump_operand_branch(instruction);
					break;
				}
				case Format::_22x:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					break;
				}
				case Format::_23x:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_register(instruction, 1);
					dump_comma();
					dump_operand_register(instruction, 2);
					break;
				}
				case Format::_30t:
				{
					dump_operand_branch(instruction);
					break;
				}
				case Format::_31i:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				case Format::_31t:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_branch(instruction);
					m_text += u8"\n";

					if (instruction.opcode_desc->opcode == 0x2b)	// packed-switch指令
						dump_operand_switch_packed_table(instruction);
					else if (instruction.opcode_desc->opcode == 0x2c)	// sparse-switch指令
						dump_operand_switch_sparse_table(instruction);
					else if (instruction.opcode_desc->opcode == 0x26)	// fill-array-data指令
						dump_operand_array_fill_table(instruction);
					else
						utils_throw();

					break;
				}
				case Format::_35c:
				{
					for(u8 j : utils::iterator_index(instruction.operands.registers))
					{
						dump_operand_register(instruction, j);
						dump_comma();
					}
					dump_operand_pool_index(instruction);
					break;
				}
				case Format::_3rc:
				{
					//准备数据
					const auto register_first = instruction.operands.register_first.value();
					const auto register_count = instruction.operands.register_count.value();

					for(u8 j : utils::iterator_index(register_count))
					{
						m_text += utils::text::format::format(u8"r%lld", register_first + j);
						dump_comma();
					}
					dump_operand_pool_index(instruction);
					break;
				}
				case Format::_51l:
				{
					dump_operand_register(instruction, 0);
					dump_comma();
					dump_operand_imm(instruction);
					break;
				}
				default: utils_throw();
				}
			}
			void dump_instruction(u8 & i)
			{
				//简单检查
				utils_assert(m_function_code.is_used[i] == 1);

				//准备数据
				const auto & instruction = utils::map(m_function_code.instructions)
					.get_existed_value(i);

				m_text += utils::u8string(u8"\t").repeat(m_tab_n);
				dump_hex_data(instruction.index, instruction.opcode_desc->format.size);
				m_text += instruction.opcode_desc->name;
				m_text += u8" ";
				dump_operands(instruction);
				m_text += u8"\n";

				//更新状态
				i += instruction.opcode_desc->format.size;
			}
		};
	}
	inline std::u8string dump_function_code(
		const s_function_code & function_code,
		u8 tab_n,
		const std::vector<u2> & code_data,
		std::function<std::u8string(utils::opcode::dalvik::common::instruction::PoolType pool_type, u8 index)> get_text_for_const_pool)
	{
		//转发
		return inner::dump_function_code::RealImpl{function_code, tab_n, code_data, get_text_for_const_pool}.execute();
	}
}