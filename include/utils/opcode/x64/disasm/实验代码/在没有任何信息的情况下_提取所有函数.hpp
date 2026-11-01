#pragma once

#include <queue>
#include <utils/queue.hpp>
#include <utils/basic/cmp_u_and_i.hpp>
#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>
#include <utils/windows/process/memory/小工具.hpp>
#include <utils/opcode/x64/disasm/modrm.hpp>
#include <utils/windows/vs.hpp>
#include <utils/opcode/capstone.hpp>
#include <utils/basic/sign_extend.hpp>
#include <utils/set.hpp>
#include <utils/map.hpp>
#include <utils/vector.hpp>
#include <utils/Stopwatch.hpp>

namespace utils::遍历
{
	template<typename T>
	void 只访问一次(const std::vector<T> & start_values, std::function<void(std::queue<T> & wait, const T value)> meet_one_value)
	{
		std::set<T> visited;
		std::queue<T> wait;
		
		utils::queue(wait).push_many(start_values);
		while(!wait.empty())
		{
			const auto value = wait.front(); wait.pop();

			if(utils::set(visited).is_exist(value)) continue;
			visited.insert(value);

			meet_one_value(wait, value);
		}
	}
}

namespace utils::opcode::x64::disasm::实验代码::在没有任何信息的情况下_提取所有函数::v2
{
	enum class 指令类型
	{
		普通,
		无效,			//ud
		间接跳转,		//间接jmp
		返回,			//ret
		直接跳转或分支,	//jmp or jcc
		超出范围,
	};
	class s_code_block
	{
	private:
		struct s_insn
		{
			uint64_t addr{};
			uint64_t size{};
			指令类型 type{};
			std::vector<uint64_t> next;
			std::vector<uint64_t> previous;
			std::vector<uint64_t> next_超出范围;
			
			bool is_valid{};
			struct 
			{
				unsigned int id{};
			}insn{};
			struct 
			{
				uint8_t opcode[4]{};
				uint8_t op_count{};
				cs_x86_op operands[2]{};	///< operands for this instruction.
				cs_x86_encoding encoding{};  ///< encoding information
			}x86{};
		};
		struct s_path
		{
			uint64_t addr{};
			std::vector<uint64_t> path;
		};
		class s_bl_info
		{
		private:
			static const uint64_t null_addr = (uint64_t)-1;
			bool m_is_need_path{};
			std::map<uint64_t, uint64_t> m_map;
		public:
			s_bl_info(bool is_need_path, uint64_t begin_addr) : m_is_need_path(is_need_path)
			{
				add_relation(null_addr, begin_addr);
			}
			void add_relation(uint64_t from, uint64_t to)
			{
				if(m_is_need_path)
				{
					utils::map(m_map).insert_if_not_exist(to, from);
				}
			}
			std::vector<uint64_t> get_path(uint64_t last, bool is_reverse) const
			{
				//简单检查
				utils_assert(m_is_need_path);

				std::vector<uint64_t> path; uint64_t cur = last;
				while(cur != null_addr)
				{
					path.push_back(cur);
					utils_assert(utils::map(m_map).is_exist(cur)); cur = m_map.at(cur);
				}

				if(is_reverse) std::reverse(path.begin(), path.end());
				return path;
			}
		};
		struct s_stack_layout
		{
			std::optional<uint64_t> stack_size;
			std::vector<x86_reg> saved_regs;

			bool 布局一致(const s_stack_layout & o) const
			{
				if (stack_size != o.stack_size) return false;
				if (saved_regs != o.saved_regs) return false;
				return true;
			}
		};
		struct s_函数参数
		{
			uint64_t n_arg{};
			uint64_t addr{};
		};
		struct s_规整函数
		{
			uint64_t stack_build{};
			s_stack_layout stack_layout{};
			std::map<uint64_t, std::set<uint64_t>> 函数参数;
			std::set<uint64_t> enters_to_stack_build;
			std::set<uint64_t> exits_to_stack_build;
		};
	private:
		utils::opcode::capstone::Engine m_engine;
		utils::opcode::capstone::Engine m_engine_detail{true};
		const utils::data & m_data;
		const std::vector<s_insn> m_insns;
		const std::map<uint64_t, s_规整函数> m_规整函数;
		const std::set<uint64_t> m_called_insns;
		const std::set<uint64_t> m_called_funcs;
	public:
		s_code_block(const utils::data & data) : m_data{data} {}
		void work()
		{
			disasm();
			link();
			规整函数();
			模块内部调用();
		}
	private:
		void disasm()
		{
			auto & insns = modifiable_reference(m_insns);
			insns.resize(m_data.size());
			for(uint64_t i = 0; i < m_data.size(); i++)
			{
				auto d = m_engine_detail.disasm(&m_data.m_data[i], m_data.size() - i, i, 1);
				if (d.count == 0)
				{
					insns[i].is_valid = false;
					insns[i].size = 1;
					continue;
				}
				utils_assert(d.count == 1);
				insns[i].is_valid = true;
				insns[i].size = d.insn[0].size;
				insns[i].insn.id = d.insn[0].id;
				const auto & x86 = d.insn[0].detail->x86;
				insns[i].x86.encoding = x86.encoding;
				insns[i].x86.op_count = x86.op_count;
				for(uint64_t k = 0; k < 2; k++)
					insns[i].x86.operands[k] = x86.operands[k];
				for(uint64_t k = 0; k < 4; k++)
					insns[i].x86.opcode[k] = x86.opcode[k];
			}
		}
		void link()
		{
			struct s_disasm
			{
				指令类型 type{};
				std::set<uint64_t> next;
			};
			auto do_disasm = [&](const uint64_t offset)->s_disasm
			{
				const auto & insn = m_insns[offset];
				if(!insn.is_valid) return {指令类型::无效};

				if (insn.insn.id == X86_INS_UD0 || insn.insn.id == X86_INS_UD2 || insn.insn.id == X86_INS_UD2B) return {指令类型::无效};
				if (insn.insn.id == X86_INS_RET || insn.insn.id == X86_INS_RETF) return {指令类型::返回};
			
				auto get_imm = [&](const cs_x86_encoding & encoding) -> uint64_t
				{
					utils_assert(encoding.imm_offset > 0);
					if (encoding.imm_size == 4)
					{
						return utils::basic::sign_extend::u32_to_u64(*(uint32_t*)&m_data.m_data[offset+encoding.imm_offset]);
					}
					else if (encoding.imm_size == 2)
					{
						return utils::basic::sign_extend::u16_to_u64(*(uint16_t*)&m_data.m_data[offset+encoding.imm_offset]);
					}
					else if (encoding.imm_size == 1)
					{
						return utils::basic::sign_extend::u8_to_u64(*(uint8_t*)&m_data.m_data[offset+encoding.imm_offset]);
					}
					else utils_throw();
				};
				auto get_jmp_dst = [&](const cs_x86_encoding & encoding)
				{
					uint64_t imm = get_imm(encoding);
					return std::set<uint64_t>
					{
						offset + insn.size + imm,
					};
				};
				auto get_jcc_dst = [&](const cs_x86_encoding & encoding)
				{
					uint64_t imm = get_imm(encoding);
					return std::set<uint64_t>
					{
						offset + insn.size,
						offset + insn.size + imm,
					};
				};

				switch(insn.insn.id)
				{
	#pragma region 跳转指令
				case X86_INS_JAE:
				case X86_INS_JA:
				case X86_INS_JBE:
				case X86_INS_JB:
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
				case X86_INS_JS:
				case X86_INS_JCXZ:
				case X86_INS_JECXZ:
				case X86_INS_JRCXZ:
				case X86_INS_JMP:
	#pragma endregion
				{
					const auto & x86 = insn.x86;

					switch(*(uint32_t*)&x86.opcode[0])
					{
					case 0xEB: case 0xE9:
						return {指令类型::直接跳转或分支, get_jmp_dst(x86.encoding)};
					case 0xFF:
						return {指令类型::间接跳转};
					case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
					case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
					case 0x800F: case 0x810F: case 0x820F: case 0x830F: case 0x840F: case 0x850F: case 0x860F: case 0x870F:
					case 0x880F: case 0x890F: case 0x8A0F: case 0x8B0F: case 0x8C0F: case 0x8D0F: case 0x8E0F: case 0x8F0F:
					case 0xE3:
						return {指令类型::直接跳转或分支, get_jcc_dst(x86.encoding)};
					default: utils_throw();
					}
				}
				default:
					return {指令类型::普通};
				}
			};

			auto & insns = modifiable_reference(m_insns);
			for(uint64_t i = 0; i < insns.size(); i++)
			{
				const auto & d = do_disasm(i);
				insns[i].addr = i;
				insns[i].type = d.type;

				for(const auto to : d.next)
				{
					if(to < m_data.size())
					{
						insns[i].next.push_back(to); insns[to].previous.push_back(i);
					}
					else
						insns[i].next_超出范围.push_back(to);
				}
				if(d.type == 指令类型::普通)
				{
					const uint64_t to = i + insns[i].size;
					if(to < m_data.size())
					{
						insns[i].next.push_back(to); insns[to].previous.push_back(i);
					}
					else
					{
						insns[i].type = 指令类型::超出范围;
						insns[i].next_超出范围.push_back(to);
					}
				}
			}
		}
		void 规整函数()
		{
			auto & 规整函数 = modifiable_reference(m_规整函数);
			std::set<uint64_t> 已认领的出口;
			for(const auto & exit : m_insns)
			{
				if(exit.type != 指令类型::返回) continue;
				if(utils::set(已认领的出口).is_exist(exit.addr)) continue;

				const auto enters = get_all_enters_and_path(exit.addr, false);
				std::set<uint64_t> 已检查过的入口;
				for(const auto & enter : enters)
				{
					if(utils::set(已检查过的入口).is_exist(enter.addr)) continue;

					uint64_t path_i{};
					auto o_stack_layout = 读取函数结尾(enter.path, path_i); if(!o_stack_layout.has_value()) continue;
					const auto & stack_layout = o_stack_layout.value();

					if (!寻找函数开头(enter.path, path_i, stack_layout)) continue;
					utils_assert(path_i > 0); utils_assert(!enter.path.empty());
					uint64_t stack_build = enter.path[path_i - 1];
						
					//确认所有出口的栈布局
					const auto exits = get_all_exits_and_path(stack_build, true); bool is_ok = true;
					for(const auto & any_exit : exits)
					{
						auto layout = 读取函数结尾(any_exit.path);
						if(!layout.has_value()) {is_ok = false; break;}
						if(!layout.value().布局一致(stack_layout)) {is_ok = false; break;}
					}

					const auto enters_to_stack_build = get_all_enters(stack_build);
					if (is_ok)
					{
						//获得最长的参数序列
						std::map<uint64_t, std::set<uint64_t>> 函数参数;
						{
							if(stack_build == 0x3a65)
							{
								int k = 0; k = k;
							}
							const auto _enters_to_stack_build = get_all_enters_and_path(stack_build, false);
							for(const auto & enter : _enters_to_stack_build)
							{
								const auto args = 读取函数参数保存(enter.path);
								函数参数[args.n_arg].insert(args.addr);
							}
						}
						
						std::set<uint64_t> exits_to_stack_build;
						for(const auto & exit : exits) utils::set(exits_to_stack_build).insert_not_exist(exit.addr);

						utils::map(规整函数).insert_not_exist(stack_build, {stack_build,
							stack_layout, std::move(函数参数), enters_to_stack_build, exits_to_stack_build});
						for(const auto & exit : exits)
							已认领的出口.insert(exit.addr);
					}
					//不管是否成功,这些入口都可以不用再看了
					utils::set(已检查过的入口).insert_many(enters_to_stack_build);
				}
			}

			std::map<uint64_t, std::set<uint64_t>> map_入口_to_规整函数;
			for(const auto & one : 规整函数)
			{
				for(const auto & 入口 : one.second.enters_to_stack_build)
				{
					utils::set(map_入口_to_规整函数[入口]).insert_not_exist(one.second.stack_build);
				}
			}
		}
		void 模块内部调用()
		{
			auto 提取call = [&](const std::set<uint64_t> & 已验证)
			{
				std::set<uint64_t> 未验证;
				for(const auto & one : 已验证)
				{
					遍历函数(one, false, [&](const s_insn & insn, const s_bl_info & info)
					{
						if (insn.is_valid && insn.insn.id == X86_INS_CALL)
						{
							utils_assert(insn.x86.op_count == 1);
							if(insn.x86.operands[0].type == X86_OP_IMM)
							{
								utils_assert(insn.x86.operands[0].size == 8 || insn.x86.operands[0].size == 4);
								未验证.insert(insn.x86.operands[0].imm);
							}
						}
					});
				}
				return 未验证;
			};
			auto verify = [&](const std::set<uint64_t> & 未验证)
			{
				std::set<uint64_t> 已验证;
				for(const auto & one : 未验证)
				{
					if(one >= m_data.m_data.size()) continue;
					const auto exits = get_all_exits_and_path(one, true);
				
					bool is_ok{true}; bool is_半ok{true};
					std::optional<s_stack_layout> first_layout;
					for(const auto & exit : exits)
					{
						uint64_t path_i{};
						auto layout = 读取函数结尾(exit.path, path_i, true); if(!layout.has_value()){is_半ok = false;break;}

						if(!寻找函数开头(exit.path, path_i, layout.value())) {is_半ok = false;break;}
						if(first_layout.has_value())
						{
							if(!first_layout.value().布局一致(layout.value())){is_ok = false;break;}
						}
						else
							first_layout = layout;
					}
					if(is_ok && is_半ok)
						已验证.insert(one);
					else if(is_半ok)
						已验证.insert(one);
				}
				return 已验证;
			};

			std::set<uint64_t> 已验证;
			for(const auto & one : m_规整函数)
			{
				已验证.insert(one.second.stack_build);
			}
			
			auto & called_insns = modifiable_reference(m_called_insns);
			auto & called_funcs = modifiable_reference(m_called_funcs);
			while(1)
			{
				auto 未验证 = 提取call(已验证);
				utils::set(未验证) -= called_insns;
				if(未验证.empty()) break;
				utils::set(called_insns).insert_many(未验证);

				已验证 = verify(未验证); utils::set(已验证) -= called_funcs; if(已验证.empty()) break;
				utils::set(called_funcs).insert_many(已验证);
			}
			
			for(const auto & one : called_insns)
			{
				bool is_exist = utils::set(called_funcs).is_exist(one);
				const auto text = dump_simple(one);
				const auto & t = text;
			}
		}
	private:
		template<typename T>
		T & modifiable_reference(const T & m_value)
		{
			return const_cast<T&>(m_value);
		}
		void 遍历函数(const uint64_t insn, bool is_need_path,
			std::function<void(const s_insn & insn, const s_bl_info & info)> meet_one_insn, std::optional<uint64_t> 不再继续 = {})
		{
			if(不再继续.has_value()) utils_assert(*不再继续 != insn);
		
			s_bl_info info{is_need_path, insn};
			utils::遍历::只访问一次<uint64_t>({insn}, [&](auto & wait, const uint64_t i)
			{
				meet_one_insn(m_insns[i], info);

				for(const auto & addr : m_insns[i].next)
				{
					utils_assert(addr < m_data.size());
					if(不再继续.has_value() && addr == *不再继续) continue;

					wait.push(addr); info.add_relation(i, addr);
				}
			});
		};
		void 反向遍历函数(const uint64_t insn, bool is_need_path,
			std::function<void(const s_insn & insn, const s_bl_info & info)> meet_one_insn)
		{
			s_bl_info info{is_need_path, insn};
			utils::遍历::只访问一次<uint64_t>({insn}, [&](auto & wait, const uint64_t i)
			{
				meet_one_insn(m_insns[i], info);

				for(const auto addr : m_insns[i].previous)
				{
					utils_assert(addr < m_data.size());
					wait.push(addr); info.add_relation(i, addr);
				}
			});
		};
		std::vector<s_path> get_all_enters_and_path(const uint64_t insn, bool is_reverse)
		{
			std::vector<s_path> enters;
			反向遍历函数(insn, true, [&](const s_insn & insn, const s_bl_info & info)
			{
				if(insn.previous.empty())
				{
					enters.push_back({insn.addr, info.get_path(insn.addr, !is_reverse)});
				}
			});
			return enters;
		}
		std::set<uint64_t> get_all_enters(const uint64_t insn)
		{
			std::set<uint64_t> enters;
			反向遍历函数(insn, false, [&](const s_insn & insn, const s_bl_info & info)
			{
				if(insn.previous.empty())
				{
					enters.insert(insn.addr);
				}
			});
			return enters;
		}
		std::vector<s_path> get_all_exits_and_path(const uint64_t insn, bool is_reverse)
		{
			std::vector<s_path> exits;
			遍历函数(insn, true, [&](const s_insn & insn, const s_bl_info & info)
			{
				if(insn.type != 指令类型::普通 && insn.type != 指令类型::直接跳转或分支)
				{
					exits.push_back({insn.addr, info.get_path(insn.addr, !is_reverse)});
				}
			});
			return exits;
		}
		std::set<uint64_t> get_all_exits(const uint64_t insn)
		{
			std::set<uint64_t> exits;
			遍历函数(insn, false, [&](const s_insn & insn, const s_bl_info & info)
			{
				if(insn.type != 指令类型::普通 && insn.type != 指令类型::直接跳转或分支)
				{
					exits.insert(insn.addr);
				}
			});
			return exits;
		}
		std::optional<s_stack_layout> 读取函数结尾(const std::vector<uint64_t> & path)
		{
			uint64_t path_i{};
			return 读取函数结尾(path, path_i);
		}
		std::optional<s_stack_layout> 读取函数结尾(const std::vector<uint64_t> & path, uint64_t & path_i, bool is_allow_空栈 = false)
		{
			utils_assert(path_i == 0);
			utils_assert(!path.empty());
			if(m_insns[path[0]].type != 指令类型::返回) return {};

			auto empty_stack = [&]() -> std::optional<s_stack_layout>
			{
				if(is_allow_空栈) return s_stack_layout{};
				else return {};
			};

			s_stack_layout stack_layout{};
			bool group_ret{};
			bool group_add_rsp{};
			while(1)
			{
				if(path_i >= path.size())
				{
					return group_ret ? empty_stack() : std::optional<s_stack_layout>{};
				}
				const auto & insn = m_insns[path[path_i]]; path_i++;
				const auto & x86 = insn.x86;

				utils_assert(insn.is_valid);
				if(false == group_ret)
				{
					if(insn.insn.id == X86_INS_RET || insn.insn.id == X86_INS_RETF) { group_ret = true; continue; }
					else return {};
				}
				else if(false == group_add_rsp)
				{
					if(insn.insn.id == X86_INS_ADD)
					{
						if (x86.op_count == 2 &&
							x86.operands[0].type == X86_OP_REG &&
							x86.operands[1].type == X86_OP_IMM &&
							x86.operands[0].reg == X86_REG_RSP )
						{
							//简单检查
							utils_assert(x86.operands[0].size == 8);
							utils_assert(x86.operands[1].size == 8);
							utils_assert((uint64_t)x86.operands[1].imm <= 0x10000);	//猜测它不会开太大

							group_add_rsp = true;
							stack_layout.stack_size = (uint64_t)x86.operands[1].imm;
							return stack_layout;
						}
						return empty_stack();
					}
					else if(insn.insn.id == X86_INS_POP)
					{
						if (x86.op_count == 1 &&
							x86.operands[0].type == X86_OP_REG)
						{
							//简单检查
							utils_assert(x86.operands[0].size == 8);

							group_add_rsp = true;
							stack_layout.saved_regs.push_back(x86.operands[0].reg);
							return stack_layout;
						}
						return empty_stack();
					}
					else return empty_stack();
				}
				else utils_throw();
			}
		}
		bool 寻找函数开头(const std::vector<uint64_t> & path, uint64_t & path_i, const s_stack_layout & stack_layout)
		{
			auto empty_stack = [&]()
			{
				return stack_layout.saved_regs.empty() && !stack_layout.stack_size.has_value();
			};

			while(1)
			{
				if(path_i >= path.size()) return empty_stack();
				const auto & insn = m_insns[path[path_i]]; path_i++;
				const auto & x86 = insn.x86;

				utils_assert(insn.is_valid);
				if(insn.insn.id == X86_INS_SUB && stack_layout.stack_size.has_value())
				{
					if (x86.op_count == 2 &&
						x86.operands[0].type == X86_OP_REG &&
						x86.operands[1].type == X86_OP_IMM &&
						x86.operands[0].reg == X86_REG_RSP )
					{
						//简单检查
						utils_assert(x86.operands[0].size == 8);
						utils_assert(x86.operands[1].size == 8);

						if((uint64_t)x86.operands[1].imm != stack_layout.stack_size.value()) return false;

						return true;
					}
					continue;
				}
				else if(insn.insn.id == X86_INS_PUSH && !stack_layout.saved_regs.empty())
				{
					if (x86.op_count == 1 &&
						x86.operands[0].type == X86_OP_REG &&
						x86.operands[0].size == 8)
					{
						if(x86.operands[0].reg != stack_layout.saved_regs[0]) continue;
						
						return true;
					}
					continue;
				}
				else continue;
			}
		}
		s_函数参数 读取函数参数保存(const std::vector<uint64_t> & path)
		{
			utils_assert(path.size() >= 1);
			uint64_t path_i{1};
			s_函数参数 函数参数{};
			while(1)
			{
				if(path_i >= path.size()) break;
				const auto & insn = m_insns[path[path_i]];
				const auto & x86 = insn.x86;

				utils_assert(insn.is_valid);
				if(insn.insn.id == X86_INS_MOV)
				{
					if (x86.op_count == 2 &&
						x86.operands[0].type == X86_OP_MEM &&
						x86.operands[1].type == X86_OP_REG &&
						x86.operands[0].mem.base == X86_REG_RSP &&
						x86.operands[0].mem.index == X86_REG_INVALID &&
						x86.operands[0].mem.segment == X86_REG_INVALID)
					{
						//简单检查
						utils_assert(x86.operands[0].size == x86.operands[1].size);
						utils_assert(
							x86.operands[0].size == 8 ||
							x86.operands[0].size == 4 ||
							x86.operands[0].size == 2 ||
							x86.operands[0].size == 1);
						utils_assert(x86.operands[1].reg != X86_REG_CH && x86.operands[1].reg != X86_REG_DH);

						static const std::map<x86_reg, int64_t> disps{
							{X86_REG_RCX, 8},
							{X86_REG_RDX, 0x10},
							{X86_REG_R8, 0x18},
							{X86_REG_R9, 0x20},
							{X86_REG_ECX, 8},
							{X86_REG_EDX, 0x10},
							{X86_REG_R8D, 0x18},
							{X86_REG_R9D, 0x20},
							{X86_REG_CX, 8},
							{X86_REG_DX, 0x10},
							{X86_REG_R8W, 0x18},
							{X86_REG_R9W, 0x20},
							{X86_REG_CL, 8},
							{X86_REG_DL, 0x10},
							{X86_REG_R8B, 0x18},
							{X86_REG_R9B, 0x20},
						};

						if(!utils::map(disps).is_exist(x86.operands[1].reg))
							break;
						if(disps.at(x86.operands[1].reg) != x86.operands[0].mem.disp)
							break;

						//函数参数.regs.push_back(x86.operands[1].reg);
						函数参数.n_arg = (std::max)(函数参数.n_arg, (uint64_t)x86.operands[0].mem.disp/8);
						path_i++;
						continue;
					}
					break;
				}
				break;
			}
			path_i--;
			utils_assert(path_i >= 0); 函数参数.addr = path[path_i];
			return 函数参数;
		}
		
		std::u8string dump_simple(const uint64_t insn)
		{
			const auto insns = dump_get_insns(insn);

			std::u8string text;
			uint64_t next_addr{(uint64_t)-1};
			for(const auto i : insns)
			{
				if(next_addr != i)
				{
					if(next_addr != (uint64_t)-1 && i < next_addr)
						text += utils::text::format::format(u8"(overlap 0x%llx)\n", next_addr - i);
					text += utils::text::format::format(u8"insn 0x%llx:\n", i);
				}

				const auto d = m_engine_detail.disasm(&m_data.m_data[i], m_data.m_data.size() - i, i, 1);
				text += utils::text::format::format(u8"    %s\n", dump_insn_text(d).c_str());
				next_addr = i + m_insns[i].size;
			}
			return text;
		}
		
	private:
		std::vector<uint64_t> dump_get_insns(const uint64_t insn)
		{
			std::vector<uint64_t> insns;
			遍历函数(insn, false, [&](const s_insn & insn, const s_bl_info & info)
			{
				insns.push_back(insn.addr);
			});
			std::sort(insns.begin(), insns.end(), [](const auto & a, const auto & b)
			{
				return a < b;
			});
			return insns;
		}
		inline std::u8string dump_insn_text(const utils::opcode::capstone::Engine::s_result & r)
		{
			if(r.count > 0)
			{
				utils_assert(r.count == 1);
				return utils::text::format::format(u8"%s %s", r.insn[0].mnemonic, r.insn[0].op_str);
			}
			else
				return u8"无效指令";
		}
	};
	inline void work()
	{
		auto blocks = utils::windows::process::memory::小工具::读取转储结果(R"(E:\zz\delete_me\240526\memory.bin.my)");
		for(const auto & b : utils::iterator_and_give_me_i(blocks))
		{
			if(b.i == 79) continue; //debug
			if(b.i == 80) continue; //debug
			//if(b.i < 167) continue; //debug

			if(utils::windows::process::memory::is_executable(b.value.info))
			{
				const auto debug_i = b.i;
				printf("%lld size=0x%llx ", debug_i, b.value.info.RegionSize);

				utils::Stopwatch stopwatch;
				s_code_block{b.value.data}.work();
				printf("used %.2lfs\n", stopwatch.get_seconds());
			}
		}
	}
}