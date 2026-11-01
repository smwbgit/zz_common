#pragma once

#include <memory>
#include <capstone/capstone.h>
#pragma comment(lib,"capstone/capstone.lib")
#include "utils/string.hpp"
#include "utils/assert.hpp"
#include "utils/IdGiver.hpp"
#include "utils/writer/optional_function.hpp"
#include "utils/queue.hpp"
#include "utils/FileWriter.hpp"

namespace utils::extract_meaning_of_code::translate::from_x86_to_llvm
{
	//把一块二进制代码翻译为llvm的ir形式
	//note: 翻译的结果可供llvm的优化器使用, 来实现去除花指令
	//note: 目前只支持x64
	//example:
	//	void work()
	//	{
	//		std::vector<uint8_t> data;
	//		{
	//			utils::FileReader input{R"(D:\binary_code.bin)"};
	//			input.read_whole_file(data);
	//		}
	//	
	//		utils::extract_meaning_of_code::translate::from_x86_to_llvm::Translator translator{R"(D:\output.ll)"};
	//	
	//		csh handle{};
	//		utils_assert(CS_ERR_OK == cs_open(CS_ARCH_X86, CS_MODE_64, &handle));
	//		utils_assert(CS_ERR_OK == cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON));
	//	
	//		const uint64_t base_address = 0x990000;
	//		uint64_t inst_offset{};
	//		while (1)
	//		{
	//			cs_insn* insn{};
	//			size_t count = cs_disasm(handle, data.data()+inst_offset, data.size()-inst_offset, base_address+inst_offset, 1, &insn);
	//			if (count != 1)
	//				break;
	//			utils_assert(count == 1);
	//	
	//			if (insn[0].detail->x86.opcode[0] == 0xe9)
	//			{
	//				inst_offset = insn[0].detail->x86.operands[0].imm - base_address;
	//				utils_assert(inst_offset < data.size());
	//			}
	//			else
	//			{
	//				translator.meet_one_inst(insn[0]);
	//	
	//				inst_offset += insn[0].size;
	//			}
	//			cs_free(insn, count);
	//		}
	//		utils_assert(CS_ERR_OK == cs_close(&handle));
	//	}
	class Translator
	{
#pragma region 私有类
	private:
		//note: 由于cs_insn里有new出来的数据, 所以在这里保存一下, 如果inst是自己"制作"出来的话
		class s_inst_queue_entry
		{
		public:
			cs_insn inst{};
			std::unique_ptr<cs_detail> detail;
		public:
			s_inst_queue_entry()
			{
				detail.reset(new cs_detail{});
				inst.detail = detail.get();
			}
			s_inst_queue_entry(const cs_insn & inst)
				: inst{inst}
			{
			}
		};
		//把一条inst指令, 或者转为几条ir指令, 或者转为几条原架构的等价指令
		class InstTranslator
		{
#pragma region 私有类
		private:
			struct s_jump_this_inst
			{
			};
			//一个ir里的栈变量
			struct s_temp_var
			{
				uint64_t index{};	//用于拼出变量名
				uint64_t size{};	//变量的大小

				s_temp_var()
				{
				}
				s_temp_var(uint64_t index, uint64_t size)
					: index{index}
					, size{size}
				{
				}
			};
			//一个ir里的栈变量, 其类型是指针
			struct s_mem_ptr
			{
				s_temp_var var;
				uint64_t size;	//指针指向的类型的大小
			};
			//表示原架构中的一个寄存器
			struct s_reg
			{
				x86_reg id;
				uint64_t size;
			};
#pragma endregion
#pragma region 成员变量
		private:
			std::vector<s_inst_queue_entry>* m_org_arch_insts{};
			std::vector<std::string>* m_ir_insts{};
			utils::IdGiver<uint64_t>* m_temp_var_index_giver{};
#pragma endregion
#pragma region 私有函数
		private:
#pragma region 变量
			//获取一个没被使用过的临时变量的序号
			uint64_t get_new_temp_var_index()
			{
				return m_temp_var_index_giver->get_one_id();
			}
			//获取指定寄存器在ir中用哪个变量表示
			const std::string & get_reg_var_name(const x86_reg reg)
			{
				static utils::map<x86_reg, std::string> table = {
					{x86_reg::X86_REG_RAX,	"rax"},
					{x86_reg::X86_REG_RBX,	"rbx"},
					{x86_reg::X86_REG_RCX,	"rcx"},
					{x86_reg::X86_REG_RDX,	"rdx"},
					{x86_reg::X86_REG_RSI,	"rsi"},
					{x86_reg::X86_REG_RDI,	"rdi"},
					{x86_reg::X86_REG_RBP,	"rbp"},
					{x86_reg::X86_REG_RSP,	"rsp"},
					{x86_reg::X86_REG_R8 ,	"r8_"},
					{x86_reg::X86_REG_R9 ,	"r9_"},
					{x86_reg::X86_REG_R10,	"r10"},
					{x86_reg::X86_REG_R11,	"r11"},
					{x86_reg::X86_REG_R12,	"r12"},
					{x86_reg::X86_REG_R13,	"r13"},
					{x86_reg::X86_REG_R14,	"r14"},
					{x86_reg::X86_REG_R15,	"r15"},
					
					{x86_reg::X86_REG_AL  ,	"rax.1"},
					{x86_reg::X86_REG_BL  ,	"rbx.1"},
					{x86_reg::X86_REG_CL  ,	"rcx.1"},
					{x86_reg::X86_REG_DL  ,	"rdx.1"},
					{x86_reg::X86_REG_SIL ,	"rsi.1"},
					{x86_reg::X86_REG_DIL ,	"rdi.1"},
					{x86_reg::X86_REG_BPL ,	"rbp.1"},
					{x86_reg::X86_REG_SPL ,	"rsp.1"},
					{x86_reg::X86_REG_R8B ,	"r8_.1"},
					{x86_reg::X86_REG_R9B ,	"r9_.1"},
					{x86_reg::X86_REG_R10B,	"r10.1"},
					{x86_reg::X86_REG_R11B,	"r11.1"},
					{x86_reg::X86_REG_R12B,	"r12.1"},
					{x86_reg::X86_REG_R13B,	"r13.1"},
					{x86_reg::X86_REG_R14B,	"r14.1"},
					{x86_reg::X86_REG_R15B,	"r15.1"},

					{x86_reg::X86_REG_AX  ,	"rax.2"},
					{x86_reg::X86_REG_BX  ,	"rbx.2"},
					{x86_reg::X86_REG_CX  ,	"rcx.2"},
					{x86_reg::X86_REG_DX  ,	"rdx.2"},
					{x86_reg::X86_REG_SI  ,	"rsi.2"},
					{x86_reg::X86_REG_DI  ,	"rdi.2"},
					{x86_reg::X86_REG_BP  ,	"rbp.2"},
					{x86_reg::X86_REG_SP  ,	"rsp.2"},
					{x86_reg::X86_REG_R8W ,	"r8_.2"},
					{x86_reg::X86_REG_R9W ,	"r9_.2"},
					{x86_reg::X86_REG_R10W,	"r10.2"},
					{x86_reg::X86_REG_R11W,	"r11.2"},
					{x86_reg::X86_REG_R12W,	"r12.2"},
					{x86_reg::X86_REG_R13W,	"r13.2"},
					{x86_reg::X86_REG_R14W,	"r14.2"},
					{x86_reg::X86_REG_R15W,	"r15.2"},

					{x86_reg::X86_REG_EAX ,	"rax.4"},
					{x86_reg::X86_REG_EBX ,	"rbx.4"},
					{x86_reg::X86_REG_ECX ,	"rcx.4"},
					{x86_reg::X86_REG_EDX ,	"rdx.4"},
					{x86_reg::X86_REG_ESI ,	"rsi.4"},
					{x86_reg::X86_REG_EDI ,	"rdi.4"},
					{x86_reg::X86_REG_EBP ,	"rbp.4"},
					{x86_reg::X86_REG_ESP ,	"rsp.4"},
					{x86_reg::X86_REG_R8D ,	"r8_.4"},
					{x86_reg::X86_REG_R9D ,	"r9_.4"},
					{x86_reg::X86_REG_R10D,	"r10.4"},
					{x86_reg::X86_REG_R11D,	"r11.4"},
					{x86_reg::X86_REG_R12D,	"r12.4"},
					{x86_reg::X86_REG_R13D,	"r13.4"},
					{x86_reg::X86_REG_R14D,	"r14.4"},
					{x86_reg::X86_REG_R15D,	"r15.4"},
				};
				return table.get_existed_value(reg);
			}
#pragma endregion
#pragma region 保存结果
			//写入一个完整的已经转换好的指令
			void write_ir_inst(const std::string & ir_inst)
			{
				m_ir_insts->push_back(ir_inst);
			}
#pragma endregion
#pragma region 原架构指令
			//生成一个表示原架构指令的结构体
			s_inst_queue_entry make_org_arch_inst(const std::string & s)
			{
				s_inst_queue_entry entry{};
				auto & inst = entry.inst;
				if (s == "sub rsp, 8")
				{
					inst.id = X86_INS_SUB;
					strcpy(inst.mnemonic, "sub");
					strcpy(inst.op_str, "rsp, 8");
					inst.detail->x86.op_count = 2;
					inst.detail->x86.operands[0].type = X86_OP_REG;
					inst.detail->x86.operands[0].size = 8;
					inst.detail->x86.operands[0].reg = X86_REG_RSP;
					inst.detail->x86.operands[1].type = X86_OP_IMM;
					inst.detail->x86.operands[1].size = 8;
					inst.detail->x86.operands[1].imm = 8;
					return entry;
				}
				else if (s == "add rsp, 8")
				{
					inst.id = X86_INS_ADD;
					strcpy(inst.mnemonic, "add");
					strcpy(inst.op_str, "rsp, 8");
					inst.detail->x86.op_count = 2;
					inst.detail->x86.operands[0].type = X86_OP_REG;
					inst.detail->x86.operands[0].size = 8;
					inst.detail->x86.operands[0].reg = X86_REG_RSP;
					inst.detail->x86.operands[1].type = X86_OP_IMM;
					inst.detail->x86.operands[1].size = 8;
					inst.detail->x86.operands[1].imm = 8;
					return entry;
				}
		
				utils_throw();
			}
			//特化
			s_inst_queue_entry make_org_arch_inst(const std::string & s, const cs_x86_op & operand)
			{
				s_inst_queue_entry entry{};
				auto & inst = entry.inst;
				if (s == "mov [rsp], operand")
				{
					inst.id = X86_INS_MOV;
					strcpy(inst.mnemonic, "mov");
					strcpy(inst.op_str, "[rsp], operand");
					inst.detail->x86.op_count = 2;
					inst.detail->x86.operands[0].type = X86_OP_MEM;
					inst.detail->x86.operands[0].size = operand.size;
					inst.detail->x86.operands[0].mem.disp = 0;
					inst.detail->x86.operands[0].mem.scale = 1;
					inst.detail->x86.operands[0].mem.segment = X86_REG_INVALID;
					inst.detail->x86.operands[0].mem.index = X86_REG_INVALID;
					inst.detail->x86.operands[0].mem.base = X86_REG_RSP;
					inst.detail->x86.operands[1] = operand;
					return entry;
				}
				else if (s == "mov operand, [rsp]")
				{
					inst.id = X86_INS_MOV;
					strcpy(inst.mnemonic, "mov");
					strcpy(inst.op_str, "operand, [rsp]");
					inst.detail->x86.op_count = 2;
					inst.detail->x86.operands[0] = operand;
					inst.detail->x86.operands[1].type = X86_OP_MEM;
					inst.detail->x86.operands[1].size = operand.size;
					inst.detail->x86.operands[1].mem.disp = 0;
					inst.detail->x86.operands[1].mem.scale = 1;
					inst.detail->x86.operands[1].mem.segment = X86_REG_INVALID;
					inst.detail->x86.operands[1].mem.index = X86_REG_INVALID;
					inst.detail->x86.operands[1].mem.base = X86_REG_RSP;
					return entry;
				}
		
				utils_throw();
			}
#pragma endregion
#pragma region 实现单一功能
			//把值进行0扩展,保存到一个临时变量里,返回这个临时变量
			s_temp_var zero_extend_to_temp_var(const s_temp_var & value, const uint64_t size)
			{
				//简单检查
				utils_assert(value.size < size);

				//准备数据
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = zext i%llu %%tmp.%llu to i%llu", temp_var_index, value.size*8, value.index, size*8);
				write_ir_inst(s);

				return {temp_var_index, size};
			}
			//生成一个临时变量,值为指定的imm
			s_temp_var make_imm_temp_var(uint64_t value, uint64_t size)
			{
				//准备数据
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = add i%llu %llu, 0", temp_var_index, size*8, value);
				write_ir_inst(s);

				return {temp_var_index, size};
			}
			//把一个值转为指针
			//size:指针指向的类型的大小
			s_mem_ptr conv_to_pointer_temp_var(const s_temp_var & value, const uint64_t size)
			{
				//简单检查
				utils_assert(value.size == 8);

				//准备数据
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = inttoptr i64 %%tmp.%llu to i%llu*", temp_var_index, value.index, size*8);
				write_ir_inst(s);

				return {{temp_var_index, 8}, size};
			}
			//写入一个用来执行二元操作的指令
			s_temp_var write_binary_op_inst(const std::string & op, const s_temp_var & op0, const s_temp_var & op1)
			{
				//简单检查
				utils_assert(op0.size == op1.size);

				//准备数据
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = %s i%llu %%tmp.%llu, %%tmp.%llu", temp_var_index, op.c_str(), op0.size*8, op0.index, op1.index);
				write_ir_inst(s);

				return {temp_var_index, op0.size};
			}
#pragma endregion
#pragma region 实现复合功能
			//返回一个临时变量.如果指定了寄存器,值为寄存器里的值,否则为0
			s_temp_var write_block_load_reg_or_0_if_null(const s_reg & reg)
			{
				if (reg.id == X86_REG_INVALID)
				{
					return make_imm_temp_var(0, reg.size);
				}
				else
				{
					return load_to_temp_var(reg);
				}
			}
			//把value的值存入reg
			void store_temp_var(const s_reg & reg, const s_temp_var & value)
			{
				//简单检查
				utils_assert(reg.size == value.size);

				//准备数据
				const auto & reg_name = get_reg_var_name(reg.id);

				utils::string s;
				s.format("store i%llu %%tmp.%llu, i%llu* %%%s, align %llu", value.size*8, value.index, value.size*8, reg_name.c_str(), value.size);
				write_ir_inst(s);
			}
			//特化: 把value存入内存
			//mem_ptr: 要存入的地址
			void store_temp_var(const s_mem_ptr & mem_ptr, const s_temp_var & value)
			{
				//简单检查
				utils_assert(mem_ptr.size == value.size);

				utils::string s;
				s.format("store i%llu %%tmp.%llu, i%llu* %%tmp.%llu, align %llu",
					value.size*8, value.index, value.size*8, mem_ptr.var.index, value.size);
				write_ir_inst(s);
			}
			//特化: 把value存入operand
			void store_temp_var(const cs_x86_op & operand, const s_temp_var & value)
			{
				if (operand.type == X86_OP_REG)
				{
					store_temp_var(s_reg{operand.reg, operand.size}, value);
				}
				else if (operand.type == X86_OP_MEM)
				{
					s_temp_var op0 = write_block_mem_operand_address(operand.mem);
					s_mem_ptr op1 = conv_to_pointer_temp_var(op0, operand.size);
					store_temp_var(op1, value);
				}
				else
				{
					utils_throw();
				}
			}
			//获取mem类型操作数的值
			//e.g.: 对于 add [1234],10 里的[1234] 可以获取 1234
			s_temp_var write_block_mem_operand_address(const x86_op_mem & mem)
			{
				if (mem.segment == X86_REG_GS)
					throw s_jump_this_inst{};

				//简单检查
				utils_assert(mem.segment == X86_REG_INVALID);

				s_temp_var _base = write_block_load_reg_or_0_if_null({mem.base, 8});
				s_temp_var _disp = make_imm_temp_var(mem.disp, 8);
				s_temp_var _index = write_block_load_reg_or_0_if_null({mem.index, 8});
				s_temp_var _scale = make_imm_temp_var(mem.scale, 8);

				//简单检查
				utils_assert(_base.size == 8);
				utils_assert(_index.size == 8);
			
				s_temp_var r_0 = write_binary_op_inst("add", _base, _disp);
		//		s_temp_var r_1 = write_binary_op_inst("imul", _index, _scale);
				s_temp_var r_2 = write_binary_op_inst("add", r_0, _index);

				//简单检查
				utils_assert(mem.scale == 1);

				return r_2;
			}
			//写入一组用来执行二元操作的指令
			void write_binary_op_block(const cs_insn & inst, const std::string & op)
			{
				//简单检查
				utils_assert(inst.detail->x86.op_count == 2);

				s_temp_var op0 = load_to_temp_var(inst.detail->x86.operands[0]);
				s_temp_var op1 = load_to_temp_var(inst.detail->x86.operands[1]);

				s_temp_var op2 = write_binary_op_inst(op, op0, op1);

				store_temp_var(inst.detail->x86.operands[0], op2);
			}
			//载入数据到临时变量
			s_temp_var load_to_temp_var(const s_reg & reg)
			{
				//准备数据
				const auto & reg_name = get_reg_var_name(reg.id);
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = load i%llu, i%llu* %%%s, align %llu", temp_var_index, reg.size*8, reg.size*8, reg_name.c_str(), reg.size);
				write_ir_inst(s);

				return {temp_var_index, reg.size};
			}
			//特化: 从内存读取
			s_temp_var load_to_temp_var(const s_mem_ptr & mem_ptr)
			{
				//准备数据
				const uint64_t temp_var_index = get_new_temp_var_index();

				utils::string s;
				s.format("%%tmp.%llu = load i%llu, i%llu* %%tmp.%llu, align %llu",
					temp_var_index, mem_ptr.size*8, mem_ptr.size*8, mem_ptr.var.index, mem_ptr.size);
				write_ir_inst(s);

				return {temp_var_index, mem_ptr.size};
			}
			//特化
			s_temp_var load_to_temp_var(const cs_x86_op & operand)
			{
				if (operand.type == X86_OP_REG)
				{
					return load_to_temp_var(s_reg{operand.reg, operand.size});
				}
				else if (operand.type == X86_OP_IMM)
				{
					return make_imm_temp_var(operand.imm, operand.size);
				}
				else if (operand.type == X86_OP_MEM)
				{
					//准备数据
					const uint64_t temp_var_index = get_new_temp_var_index();
					const uint64_t size = operand.size;

					s_temp_var op0 = write_block_mem_operand_address(operand.mem);
					s_mem_ptr op1 = conv_to_pointer_temp_var(op0, operand.size);
					s_temp_var op2 = load_to_temp_var(op1);

					return op2;
				}
				else
				{
					utils_throw();
				}
			}
#pragma endregion
#pragma region 主逻辑
			//翻译
			void _translate(_In_ const cs_insn & inst)
			{
				switch(inst.id)
				{
				case x86_insn::X86_INS_AND:
				{
					write_binary_op_block(inst, "and");
					break;
				}
				case x86_insn::X86_INS_OR:
				{
					write_binary_op_block(inst, "or");
					break;
				}
				case x86_insn::X86_INS_ADD:
				case x86_insn::X86_INS_ADC:	//todo: 修正此指令
				{
					write_binary_op_block(inst, "add");
					break;
				}
				case x86_insn::X86_INS_SUB:
				case x86_insn::X86_INS_SBB:	//todo: 修正此指令
				{
					write_binary_op_block(inst, "sub");
					break;
				}
				case x86_insn::X86_INS_IMUL:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 2);

					write_binary_op_block(inst, "mul");
					break;
				}
				case x86_insn::X86_INS_XOR:
				{
					write_binary_op_block(inst, "xor");
					break;
				}
				case x86_insn::X86_INS_MOV:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 2);

					s_temp_var op1 = load_to_temp_var(inst.detail->x86.operands[1]);
					store_temp_var(inst.detail->x86.operands[0], op1);
					break;
				}
				case x86_insn::X86_INS_XCHG:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 2);

					s_temp_var op0 = load_to_temp_var(inst.detail->x86.operands[0]);
					s_temp_var op1 = load_to_temp_var(inst.detail->x86.operands[1]);
					store_temp_var(inst.detail->x86.operands[1], op0);
					store_temp_var(inst.detail->x86.operands[0], op1);
					break;
				}
				case x86_insn::X86_INS_MOVABS:
				{
					//简单检查
					utils_assert(inst.detail->x86.opcode[0] >= 0xb8 && inst.detail->x86.opcode[0] <= 0xbf);

					//转为mov指令

					s_inst_queue_entry inst_0{inst};
					inst_0.inst.id = x86_insn::X86_INS_MOV;
					strcpy(inst_0.inst.mnemonic, "mov");
					m_org_arch_insts->push_back(std::move(inst_0));
					break;
				}
				case x86_insn::X86_INS_LEA:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 2);
					utils_assert(inst.detail->x86.operands[1].type == X86_OP_MEM);
					utils_assert(inst.detail->x86.operands[1].size == 8);

					s_temp_var op0 = write_block_mem_operand_address(inst.detail->x86.operands[1].mem);
					store_temp_var(inst.detail->x86.operands[0], op0);
					break;
				}
				case x86_insn::X86_INS_MOVZX:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 2);

					s_temp_var op1 = load_to_temp_var(inst.detail->x86.operands[1]);

					//简单检查
					utils_assert(inst.detail->x86.operands[0].size > op1.size);

					s_temp_var op2 = zero_extend_to_temp_var(op1, inst.detail->x86.operands[0].size);
					store_temp_var(inst.detail->x86.operands[0], op2);
					break;
				}
				case x86_insn::X86_INS_PUSH:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 1);

					//转为两条指令
					//sub rsp, 8
					//mov [rsp], operand
			
					auto _inst0 = make_org_arch_inst("sub rsp, 8");
					auto _inst1 = make_org_arch_inst("mov [rsp], operand", inst.detail->x86.operands[0]);

					m_org_arch_insts->push_back(std::move(_inst0));
					m_org_arch_insts->push_back(std::move(_inst1));
					break;
				}
				case x86_insn::X86_INS_POP:
				{
					//简单检查
					utils_assert(inst.detail->x86.op_count == 1);

					//转为两条指令
					//mov operand, [rsp]
					//add rsp, 8
			
					auto inst_0 = make_org_arch_inst("mov operand, [rsp]", inst.detail->x86.operands[0]);
					auto inst_1 = make_org_arch_inst("add rsp, 8");

					m_org_arch_insts->push_back(std::move(inst_0));
					m_org_arch_insts->push_back(std::move(inst_1));
					break;

#if 0
			s_temp_var op2 = load_to_temp_var(s_reg{x86_reg::X86_REG_RSP, 8});
			s_temp_var op3 = make_imm_temp_var(8, 8);
			s_temp_var op4 = binary_op_to_temp_var("add", op2, op3);
			store_temp_var(s_reg{x86_reg::X86_REG_RSP, 8}, op4);
			s_mem_ptr op5 = conv_to_pointer_temp_var(op4, inst.detail->x86.operands[0].size);
			s_temp_var op6 = load_to_temp_var(op5);
			store_temp_var(inst.detail->x86.operands[0], op6);
#endif
				}
				case x86_insn::X86_INS_STC:		//todo: 修正此指令
				case x86_insn::X86_INS_CLC:		//todo: 修正此指令
				case x86_insn::X86_INS_CMC:		//todo: 修正此指令
				case x86_insn::X86_INS_SHL:		//todo: 修正此指令
				case x86_insn::X86_INS_SHR:		//todo: 修正此指令
				case x86_insn::X86_INS_SAR:		//todo: 修正此指令
				case x86_insn::X86_INS_RCL:		//todo: 修正此指令
				case x86_insn::X86_INS_ROR:		//todo: 修正此指令
				case x86_insn::X86_INS_PUSHFQ:	//todo: 修正此指令
				case x86_insn::X86_INS_POPFQ:	//todo: 修正此指令
				{
					throw s_jump_this_inst{};
				}
				case x86_insn::X86_INS_JMP:		//todo: 修正此指令
				{
					throw s_jump_this_inst{};
				}
				case x86_insn::X86_INS_ROL:		//todo: 修正此指令
				{
#if 0
			//简单检查
			utils_assert(inst.detail->x86.op_count == 2);

			s_temp_var op0 = load_to_temp_var(inst.detail->x86.operands[0]);
			s_temp_var op1 = load_to_temp_var(inst.detail->x86.operands[1]);

			s_temp_var op2 = binary_op_to_temp_var("shl", op0, op1);
			s_temp_var op3 = make_imm_temp_var(op0.size*8, op1.size);
			s_temp_var op4 = binary_op_to_temp_var("sub", op3, op1);
			s_temp_var op5 = binary_op_to_temp_var("lshr", op0, op4);
			s_temp_var op6 = binary_op_to_temp_var("and", op2, op5);

			store_temp_var(inst.detail->x86.operands[0], op6);
#endif
					throw s_jump_this_inst{};
				}
				default:
					utils_throw();
				}
			}

#pragma endregion
#pragma endregion
#pragma region 公共函数
		public:
			//翻译
			//inst: 要翻译的指令
			//org_arch_insts: 等价的原架构指令
			//ir_insts: 翻译出来的ir语句
			//temp_var_index_giver: 临时变量的序号生成器
			bool translate(
				_Out_ std::vector<s_inst_queue_entry> & org_arch_insts,
				_Out_ std::vector<std::string> & ir_insts,
				_In_ const cs_insn & inst,
				_In_ utils::IdGiver<uint64_t> & temp_var_index_giver)
			{
				//保存
				m_org_arch_insts = &org_arch_insts;
				m_ir_insts = &ir_insts;
				m_temp_var_index_giver = &temp_var_index_giver;

				//清空输出
				m_org_arch_insts->clear();
				m_ir_insts->clear();

				try
				{
					_translate(inst);
					return true;
				}
				catch(s_jump_this_inst &)
				{
					return false;
				}
			}
#pragma endregion
		};
#pragma endregion
#pragma region 成员变量
	private:
//		utils::FileWriter m_output;
		utils::writer::optional_function::TabInsert<utils::FileWriter> m_output;
		utils::queue<s_inst_queue_entry> m_inst_queue;		//增加一个指令队列,方便指令替换. 比如把"push [eax]" 换为 "sub rsp, 8" 和 "mov [rsp], [eax]"
		utils::IdGiver<uint64_t> m_temp_var_index_giver;
#pragma endregion
#pragma region 私有函数
	private:
#pragma region 写入操作
		//写上ir文件的开头
		void write_head()
		{
			m_output.write_text(R"(
; ModuleID = 'main.c'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @main_func(
i64* %rax,
i64* %rbx,
i64* %rcx,
i64* %rdx,
i64* %rsi,
i64* %rdi,
i64* %rbp,
i64* %rsp,
i64* %r8_,
i64* %r9_,
i64* %r10,
i64* %r11,
i64* %r12,
i64* %r13,
i64* %r14,
i64* %r15) #0 {

  %rax.4 = bitcast i64* %rax to i32*
  %rbx.4 = bitcast i64* %rbx to i32*
  %rcx.4 = bitcast i64* %rcx to i32*
  %rdx.4 = bitcast i64* %rdx to i32*
  %r8_.4 = bitcast i64* %r8_ to i32*
  %r9_.4 = bitcast i64* %r9_ to i32*
  %r10.4 = bitcast i64* %r10 to i32*
  %r11.4 = bitcast i64* %r11 to i32*
  %r12.4 = bitcast i64* %r12 to i32*
  %r13.4 = bitcast i64* %r13 to i32*
  %r14.4 = bitcast i64* %r14 to i32*
  %r15.4 = bitcast i64* %r15 to i32*
  %rsi.4 = bitcast i64* %rsi to i32*
  %rdi.4 = bitcast i64* %rdi to i32*
  %rbp.4 = bitcast i64* %rbp to i32*
  %rsp.4 = bitcast i64* %rsp to i32*

  %rax.2 = bitcast i64* %rax to i16*
  %rbx.2 = bitcast i64* %rbx to i16*
  %rcx.2 = bitcast i64* %rcx to i16*
  %rdx.2 = bitcast i64* %rdx to i16*
  %r8_.2 = bitcast i64* %r8_ to i16*
  %r9_.2 = bitcast i64* %r9_ to i16*
  %r10.2 = bitcast i64* %r10 to i16*
  %r11.2 = bitcast i64* %r11 to i16*
  %r12.2 = bitcast i64* %r12 to i16*
  %r13.2 = bitcast i64* %r13 to i16*
  %r14.2 = bitcast i64* %r14 to i16*
  %r15.2 = bitcast i64* %r15 to i16*
  %rsi.2 = bitcast i64* %rsi to i16*
  %rdi.2 = bitcast i64* %rdi to i16*
  %rbp.2 = bitcast i64* %rbp to i16*
  %rsp.2 = bitcast i64* %rsp to i16*

  %rax.1 = bitcast i64* %rax to i8*
  %rbx.1 = bitcast i64* %rbx to i8*
  %rcx.1 = bitcast i64* %rcx to i8*
  %rdx.1 = bitcast i64* %rdx to i8*
  %r8_.1 = bitcast i64* %r8_ to i8*
  %r9_.1 = bitcast i64* %r9_ to i8*
  %r10.1 = bitcast i64* %r10 to i8*
  %r11.1 = bitcast i64* %r11 to i8*
  %r12.1 = bitcast i64* %r12 to i8*
  %r13.1 = bitcast i64* %r13 to i8*
  %r14.1 = bitcast i64* %r14 to i8*
  %r15.1 = bitcast i64* %r15 to i8*
  %rsi.1 = bitcast i64* %rsi to i8*
  %rdi.1 = bitcast i64* %rdi to i8*
  %rbp.1 = bitcast i64* %rbp to i8*
  %rsp.1 = bitcast i64* %rsp to i8*

)");

			//设置缩进
			m_output.tab_insert.change_tab_count(1);
			m_output.tab_insert.set_tab_text("  ");
		}
		//写上ir文件的结尾
		void write_tail()
		{
			//简单检查
			utils_assert(1 == m_output.tab_insert.get_tab_count());

			m_output.write_text(R"(  ret i32 0
}

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Debian clang version 11.1.0-4"}
)");
		}
		//写入一个注释
		void write_note(const cs_insn & inst)
		{
			utils::string note;
			note.format("%p:\t%s\t\t%s", (void*)inst.address, inst.mnemonic, inst.op_str);
			m_output.tab_insert.write_one_line(";" + note);
		}
		//写入一个空行
		void write_one_empty_line()
		{
			m_output.tab_insert.write_one_line("");
		}
		//写入ir语句
		void write_ir_insts(_In_ const std::vector<std::string> & ir_insts)
		{
			for (const auto & one : ir_insts)
			{
				m_output.tab_insert.write_one_line(one);
			}
		}
#pragma endregion
#pragma region 解析
		//翻译一条指令或其某条等价指令
		void look_this_inst(const cs_insn & inst)
		{
			//写入注释
			write_note(inst);
			
			InstTranslator translator;

			//准备保存结果的地方
			std::vector<s_inst_queue_entry> org_arch_insts;	//等价的原架构指令
			std::vector<std::string> ir_insts;	//ir指令

			//翻译为ir
			if (translator.translate(org_arch_insts, ir_insts, inst, m_temp_var_index_giver))
			{
				//成功完成
				//a.把生成的对应ir语句写入文件
				//b.把原架构指令放入队列
				
				//简单检查: a和b不会同时发生
				utils_assert(
					org_arch_insts.empty() && false == ir_insts.empty() ||
					false == org_arch_insts.empty() && ir_insts.empty());

				//写入ir语句
				write_ir_insts(ir_insts);

				//放入原架构等价指令
				for (auto & one : org_arch_insts)
				{
					m_inst_queue.push(std::move(one));
				}
			}
			else
			{
				//如果该指令现在还不支持,则不写入
			}
			
			//写入一个空行,和下一条指令分隔开
			write_one_empty_line();
		}
#pragma endregion
#pragma endregion
#pragma region 公共函数
	public:
		Translator(_In_ const std::string & filename)
			: m_output{filename}
		{
			write_head();
		}
		//写入结束信息
		~Translator()
		{
			write_tail();
		}
	public:
		//翻译一条指令
		void meet_one_inst(const cs_insn & inst)
		{
			//简单检查
			utils_assert(m_inst_queue.empty());

			m_inst_queue.push({inst});

			m_inst_queue.pop_all([&](const s_inst_queue_entry & one)
			{
				look_this_inst(one.inst);
			});
		}
#pragma endregion
	};
}