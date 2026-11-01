#pragma once

#include <utils/binary/code/capstone.hpp>
#include <utils/binary/code/concepts/VirtualAddressReader.hpp>
#include <utils/DataReader.hpp>
#include <utils/vector.hpp>
#include <utils/map.hpp>
#include <utils/algorithm/graph.hpp>

//基本块
namespace utils::binary::code::basic_block
{
	//提示信息
	class TipManager
	{
#pragma region 函数信息
	public:
		//函数信息
		struct s_function
		{
			uint64_t m_entry{};
			std::optional<std::u8string> m_name;
			std::optional<bool> m_is_return;	//是否会返回
			std::optional<uint64_t> m_param_count{};	//参数个数
		};
	private:
		std::map<uint64_t, s_function> m_functions;
	public:
		//设置函数信息
		s_function & set_function(uint64_t func_entry)
		{
			return m_functions[func_entry];
		}
		//查询函数参数的个数
		void get_function_param_count(uint64_t func_entry, bool & is_known, uint64_t & param_count) const
		{
			//不存在函数信息
			if(!utils::map(m_functions).is_exist(func_entry))
			{
				is_known = false; return;
			}
			
			//不存在个数信息
			const auto & func = utils::map(m_functions).get_existed_value(func_entry);
			if (!func.m_param_count.has_value())
			{
				is_known = false; return;
			}

			//给出
			is_known = true;
			param_count = func.m_param_count.value();
		}
		//查询函数是否返回
		void is_function_return(uint64_t func_entry, bool & is_known, bool & is_return) const
		{
			//不存在函数信息
			if(!utils::map(m_functions).is_exist(func_entry))
			{
				is_known = false; return;
			}
			
			//不存在返回信息
			const auto & func = utils::map(m_functions).get_existed_value(func_entry);
			if (!func.m_is_return.has_value())
			{
				is_known = false; return;
			}

			//给出
			is_known = true;
			is_return = func.m_is_return.value();
		}
#pragma endregion
	};
	//基本块的详细信息
	struct Detail
	{
	public:
		struct s_register_use_position
		{
			std::vector<uint64_t> m_read_positions;	//相对于block_addr的地址偏移。数组是排好序的
			std::vector<uint64_t> m_write_positions;
			std::vector<uint64_t> m_assign_positions;

			struct s_position
			{
				bool m_is_read{};
				bool m_is_write{};
				bool m_is_assign{};
			};
			std::map<uint64_t, s_position> m_positions;	//相对于block_addr的地址偏移, 每个语句，都会存在一条
		};

	public:
		//节点关系
		std::set<uint64_t> m_upstream;		//所有上游节点
		std::set<uint64_t> m_downstream;	//所有下游节点
		std::set<uint64_t> m_immediate_predecessor;	//直接前继节点
		std::set<uint64_t> m_immediate_successor;	//直接后继节点. 值和BasicBlock.m_next相同
		bool m_is_loop_node{};	//当前节点是否为循环节点
		
		//代码细节
		std::map<uint64_t /* register_id */, s_register_use_position> m_register_use_positions;
	};
	//基本块
	struct BasicBlock
	{
	private:
		using s_instruction_duplication = utils::binary::code::capstone::s_instruction_duplication;
		
	public:
		//基本信息
		uint64_t m_addr{};
		uint64_t m_size{};
		std::set<uint64_t> m_next;
		
		//其中的指令
		std::u8string m_text;
		std::map<uint64_t /* pos */, s_instruction_duplication> m_instructions;

		//更多推导信息
		std::optional<Detail> m_detail;

	public:
		//简便写法
		Detail* operator ->()
		{
			return &m_detail.value();
		}
		//特化
		const Detail* operator ->() const
		{
			return &m_detail.value();
		}
	};
	//基本块的图
	class Graph
	{
	private:
		uint64_t m_entry{};
		std::map<uint64_t, BasicBlock> m_basic_blocks;
		const basic_block::TipManager & m_tip_manager;

	public:
		Graph(uint64_t entry, const std::map<uint64_t, BasicBlock> & basic_blocks,
			const basic_block::TipManager & tip_manager)
			: m_entry{entry}
			, m_basic_blocks{basic_blocks}
			, m_tip_manager{tip_manager}
		{
			make_detail();
		}
		std::map<uint64_t, BasicBlock> & get_basic_blocks()
		{
			return m_basic_blocks;
		}
		const std::map<uint64_t, BasicBlock> & get_basic_blocks() const
		{
			return m_basic_blocks;
		}
		//查找basic_block
		BasicBlock & look_up_basic_block(uint64_t block_addr)
		{
			return utils::map(m_basic_blocks).get_existed_value(block_addr);
		}
		//特化
		const BasicBlock & look_up_basic_block(uint64_t block_addr) const
		{
			return utils::map(m_basic_blocks).get_existed_value(block_addr);
		}
		//获得入口地址
		uint64_t get_entry() const
		{
			return m_entry;
		}
	private:
		//获得基本块的更多推导信息
		void make_detail()
		{
			//直接后继节点
			for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
			{
				//创建节点信息
				utils_assert(!block.m_detail);
				block.m_detail = Detail{};

				//填写
				block.m_detail->m_immediate_successor = block.m_next;
			}

			//直接前继节点
			for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
			{
				//对当前节点的所有后继结点进行填写
				for (const auto next : block.m_detail->m_immediate_successor)
				{
					//查找后继节点
					auto & next_block = utils::map(m_basic_blocks).get_existed_value(next);
				
					//填写
					utils::set(next_block.m_detail->m_immediate_predecessor).insert_not_exist(block.m_addr);
				}
			}

			//准备函数
			auto get_downstream = [this](uint64_t block_addr)
			{
				//查找节点
				auto & block = utils::map(m_basic_blocks).get_existed_value(block_addr);

				//查找所有可达节点
				std::set<uint64_t> visited;
				utils::algorithm::graph::BreadthFirstSearch<uint64_t> search;
				search.m_entry = block.m_detail->m_immediate_successor;
				search.m_meet_one_node = [&visited, this](const auto block_addr)
				{
					//记录
					utils::set(visited).insert_not_exist(block_addr);

					//查找节点
					const auto & block = utils::map(m_basic_blocks).get_existed_value(block_addr);

					//给出后继
					return block.m_detail->m_immediate_successor;
				};
				search.start();

				//给出
				return visited;
			};
		
			//所有下游节点
			//todo 看看这里需不需要，以及还有没有更好的办法
			for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
			{
				//填写
				block.m_detail->m_downstream = get_downstream(block.m_addr);
			}
		
			//所有上游节点
			for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
			{
				//通过下游关系，填写上游信息
				for(const auto downstream_addr : block.m_detail->m_downstream)
				{
					//查找节点
					auto & downstream_block = utils::map(m_basic_blocks).get_existed_value(downstream_addr);
			
					//填写
					utils::set(downstream_block.m_detail->m_upstream).insert_not_exist(block.m_addr);
				}
			}

			//是否为循环节点
			for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
			{
				//查找节点
				auto & detail = *block.m_detail;

				//填写
				detail.m_is_loop_node = utils::set(detail.m_downstream).is_exist(block.m_addr);
			}
		
			//使用寄存器的情况
			{
				utils::binary::code::capstone::RegisterUseManager use_manager;
				use_manager.m_get_function_param_count = [this](uint64_t func_entry, bool & is_known, uint64_t & param_count)
				{
					m_tip_manager.get_function_param_count(func_entry, is_known, param_count);
				};
				for(const auto register_id : utils::binary::code::capstone::
					register_id::get_all_64bit_general_purpose_register())
				{
					for(auto & block : utils::map(m_basic_blocks).iterator_seconds())
					{
						//查找节点
						auto & detail = *block.m_detail;
						auto & register_use_positions = detail.m_register_use_positions[register_id];

						for(const auto & instruction : block.m_instructions)
						{
							//获得使用情况
							use_manager.m_instruction = &instruction.second;
							bool is_read = use_manager.is_use_this_64bit_general_purpose_register(register_id, CS_AC_READ);
							bool is_write = use_manager.is_use_this_64bit_general_purpose_register(register_id, CS_AC_WRITE);
							
							//保存
							Detail::s_register_use_position::s_position position;
							position.m_is_read = is_read;
							position.m_is_write = is_write;
							position.m_is_assign = [&]()
							{
								if(instruction.second.m_insn.id == X86_INS_CALL)
									return is_write;	//所写入的，均为随机/不可预测值，所以，当做赋值语句
								else
									return is_write && !is_read;	//对于普通指令，排除掉像"add rax, 2"这样的语句
							}();
							utils::map(register_use_positions.m_positions).insert_not_exist(
								instruction.first, position);

							//保存
							if(position.m_is_read)
								register_use_positions.m_read_positions.push_back(instruction.first);
							if(position.m_is_write)
								register_use_positions.m_write_positions.push_back(instruction.first);
							if(position.m_is_assign)
								register_use_positions.m_assign_positions.push_back(instruction.first);
						}

						//简单检查: 已排序
						utils_assert(utils::vector(register_use_positions.m_read_positions).is_sorted());
						utils_assert(utils::vector(register_use_positions.m_write_positions).is_sorted());
						utils_assert(utils::vector(register_use_positions.m_assign_positions).is_sorted());
					}
				}
			}
		}
	};
}
//生成图
namespace utils::binary::code::basic_block::make
{
	//划分出基本块
	class Division
	{
	private:
		using BasicBlock = basic_block::BasicBlock;
		using Graph = basic_block::Graph;
	private:
		utils::DataReader & m_data_reader;
		const basic_block::TipManager & m_tip_manager;
		utils::binary::code::capstone::Engine m_capstone{true};
	public:
		Division(utils::DataReader & data_reader, const basic_block::TipManager & tip_manager)
			: m_data_reader{data_reader}
			, m_tip_manager{tip_manager}
		{
		}
		//获得一个正常编译后的函数的所有基本块
		//note: 正常编译是指由编译器编译生成，而不是手写或混淆过的
		//function_entry: 函数入口地址
		Graph get_normal(uint64_t function_entry)
		{
			//粗略划分
			std::vector<BasicBlock> blocks;
			{
				utils::algorithm::graph::BreadthFirstSearch<uint64_t> search;
				search.m_entry = {function_entry};
				search.m_meet_one_node = [this, &blocks](const uint64_t block_addr)
				{
					const auto block = get_one_block(block_addr);
					blocks.push_back(block);
					return block.m_next;
				};
				search.start();
			}

			//把被指到中间的块，分成多个块
			std::vector<BasicBlock> blocks_2;
			{
				//统计所有next指向的位置
				std::set<uint64_t> all_next;
				for(const auto & block : blocks)
					utils::set(all_next).insert_many(block.m_next);

				//准备函数
				auto is_next_in_it = [](
					const BasicBlock & block,
					const std::set<uint64_t> & all_next,
					std::vector<uint64_t> & next_in_it)
				{
					//todo 看看还有没有更好更快的办法

					//查看块内的每个字节
					next_in_it.clear();
					for(uint64_t i = 0; i < block.m_size; i++)
					{
						//不检查第一个
						if(!i) continue;

						//没有跳过来的
						if(!utils::set(all_next).is_exist(block.m_addr+i))
							continue;

						//简单检查: 跳到了指令的第一个字节
						utils_assert(utils::map(block.m_instructions).is_exist(i));

						//保存
						next_in_it.push_back(i);
					}

					return !next_in_it.empty();
				};
				auto break_it = [this](
					const BasicBlock & block,
					const std::vector<uint64_t>& next_in_it,
					std::vector<BasicBlock> & blocks_2)
				{
					//简单检查
					utils_assert(!next_in_it.empty());

					//准备函数
					auto make_new_block = [&blocks_2, &block, this](
						uint64_t i, uint64_t size,
						const std::set<uint64_t> & next)
					{
						BasicBlock sub_block{};
						sub_block.m_addr = block.m_addr+i;
						sub_block.m_size = size;
						sub_block.m_next = next;
						//复制汇编指令
						for(uint64_t _i = 0; _i < size; )
						{
							const auto & instruction = utils::map(block.m_instructions).get_existed_value(i+_i);
							utils::map(sub_block.m_instructions).insert_not_exist(_i, instruction);
							_i += instruction.m_insn.size;

							//简单检查
							utils_assert(_i <= size);
						}
						//生成新的汇编文本
						for(const auto & instruction : sub_block.m_instructions)
						{
							sub_block.m_text += (const char8_t*)instruction.second.m_insn.mnemonic;
							sub_block.m_text += u8" ";
							sub_block.m_text += (const char8_t*)instruction.second.m_insn.op_str;
							sub_block.m_text += u8"\n";
						}
						blocks_2.push_back(sub_block);
					};

					//每个跳入都形成一个新块
					uint64_t last_i = 0;
					for(const auto one : next_in_it)
					{
						make_new_block(last_i, one-last_i, {block.m_addr+one});
						last_i = one;
					}

					//再加最后一个块
					make_new_block(last_i, block.m_size-last_i, block.m_next);
				};
				auto add_it = [](
					const BasicBlock & block,
					std::vector<BasicBlock> & blocks_2)
				{
					blocks_2.push_back(block);
				};

				//检查每个块，看是否有next指向了此块的中间
				for(const auto & block : blocks)
				{
					//有的话，拆分它
					std::vector<uint64_t> next_in_it;
					if(is_next_in_it(block, all_next, next_in_it))
					{
						break_it(block, next_in_it, blocks_2);
					}
					//没有则直接添加
					else
					{
						add_it(block, blocks_2);
					}
				}
			}

			//变为map, 去掉重复的
			//todo: 看看流程是否需要优化，不产生重复的块
			std::map<uint64_t, BasicBlock> blocks_3;
			for(const auto & one : blocks_2)
			{
				if(utils::map(blocks_3).is_exist(one.m_addr))
				{
					//简单检查: 块的信息相同
					const auto & other = utils::map(blocks_3).get_existed_value(one.m_addr);
					utils_assert(one.m_addr == other.m_addr);
					utils_assert(one.m_size == other.m_size);
					utils_assert(one.m_next == other.m_next);

					continue;
				}

				utils::map(blocks_3).insert_not_exist(one.m_addr, one);
			}
			return Graph{function_entry, blocks_3, m_tip_manager};
		}
	private:
		BasicBlock get_one_block(uint64_t block_addr)
		{
			m_data_reader.set_read_position(block_addr);
			uint64_t readed_size{};
			BasicBlock block;
			while(1)
			{
				//读取指令字节
				utils::data data;
				m_data_reader.read_boundary(data, 20);
				utils_assert(!data.empty());
				
				//反汇编指令
				auto insn = m_capstone.disasm(data.addr(), data.size(), block_addr + readed_size, 1);
				utils_assert(insn.count == 1);

				//保存指令
				block.m_text += (const char8_t*)insn.insn[0].mnemonic;
				block.m_text += u8" ";
				block.m_text += (const char8_t*)insn.insn[0].op_str;
				block.m_text += u8"\n";
				utils::map(block.m_instructions).insert_not_exist(readed_size,
					m_capstone.get_instruction_duplication(insn.insn[0]));

				//更新状态
				readed_size += insn.insn[0].size;
				m_data_reader.set_read_position(block_addr + readed_size);

				//基本块结束
				if (is_block_end(insn.insn[0]))
				{
					block.m_addr = block_addr;
					block.m_size = readed_size;
					block.m_next = get_next(insn.insn[0]);
					return block;
				}
			}
		}
		std::set<uint64_t> get_next(const cs_insn & insn)
		{
			switch(insn.id)
			{
			case X86_INS_CALL:	//比如，当调用了一个不返回的函数
				return {};
			case X86_INS_RET:
				return {};
			case X86_INS_JMP:
			{
				//简单检查
				utils_assert(insn.detail->x86.op_count == 1);
				utils_assert(insn.detail->x86.operands[0].type == X86_OP_IMM);
				utils_assert(insn.detail->x86.operands[0].size == 8);

				return {(uint64_t)insn.detail->x86.operands[0].imm};
			}
			case X86_INS_JAE:
			case X86_INS_JA:
			case X86_INS_JBE:
			case X86_INS_JB:
			case X86_INS_JCXZ:
			case X86_INS_JECXZ:
			case X86_INS_JE:
			case X86_INS_JGE:
			case X86_INS_JG:
			case X86_INS_JLE:
			case X86_INS_JL:
			case X86_INS_JNE:
			case X86_INS_JNO:
			case X86_INS_JNP:
			case X86_INS_JNS:
			case X86_INS_JO:
			case X86_INS_JP:
			case X86_INS_JRCXZ:
			case X86_INS_JS:
			{
				//简单检查
				utils_assert(insn.detail->x86.op_count == 1);
				utils_assert(insn.detail->x86.operands[0].type == X86_OP_IMM);
				utils_assert(insn.detail->x86.operands[0].size == 8);

				return {
					(uint64_t)insn.detail->x86.operands[0].imm,
					insn.address + insn.size
				};
			}
			}

			utils_throw();
		}
		bool is_block_end(const cs_insn & insn)
		{
			//调用了不返回的函数
			if (insn.id == X86_INS_CALL)
			{
				//简单检查
				utils_assert(insn.detail->x86.op_count == 1);
				utils_assert(insn.detail->x86.operands[0].type == X86_OP_IMM);
				utils_assert(insn.detail->x86.operands[0].size == 8);
				
				//不返回的函数
				const auto func_addr = (uint64_t)insn.detail->x86.operands[0].imm;
				bool is_known{};
				bool is_return{};
				m_tip_manager.is_function_return(func_addr, is_known, is_return);
				if(is_known && !is_return)
					return true;
			}

			//一般的块结束语句
			switch(insn.id)
			{
			case X86_INS_RET:
			case X86_INS_JMP:
			case X86_INS_JAE:
			case X86_INS_JA:
			case X86_INS_JBE:
			case X86_INS_JB:
			case X86_INS_JCXZ:
			case X86_INS_JECXZ:
			case X86_INS_JE:
			case X86_INS_JGE:
			case X86_INS_JG:
			case X86_INS_JLE:
			case X86_INS_JL:
			case X86_INS_JNE:
			case X86_INS_JNO:
			case X86_INS_JNP:
			case X86_INS_JNS:
			case X86_INS_JO:
			case X86_INS_JP:
			case X86_INS_JRCXZ:
			case X86_INS_JS:
				return true;
			}
			
			//简单检查
			utils_assert(!cs_insn_group(m_capstone.m_handle, &insn, X86_GRP_JUMP));
			utils_assert(!cs_insn_group(m_capstone.m_handle, &insn, X86_GRP_RET));
			utils_assert(!cs_insn_group(m_capstone.m_handle, &insn, X86_GRP_IRET));

			return false;
		}
	};

	//简便函数: 对于一个指定函数，生成基本块的图
	inline basic_block::Graph for_function(utils::DataReader & reader, uint64_t function_entry,
		const basic_block::TipManager & tip_manager)
	{
		Division division{reader, tip_manager};
		return division.get_normal(function_entry);
	}
}