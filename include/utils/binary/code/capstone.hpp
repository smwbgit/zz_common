#pragma once

#include <optional>
#include <vector>
#include <utils/libs/capstone/4.0.2/all.hpp>
#include <utils/map.hpp>
#include <utils/set.hpp>

namespace utils::binary::code::capstone
{
	namespace register_id
	{
		inline auto get_all_64bit_general_purpose_register()
		{
			static std::set<uint64_t> all = {
				X86_REG_RAX,
				X86_REG_RBX,
				X86_REG_RCX,
				X86_REG_RDX,
				X86_REG_RSI,
				X86_REG_RDI,
				X86_REG_RBP,
				X86_REG_RSP,
				X86_REG_R8,
				X86_REG_R9,
				X86_REG_R10,
				X86_REG_R11,
				X86_REG_R12,
				X86_REG_R13,
				X86_REG_R14,
				X86_REG_R15,
			};
			return all;
		}
		inline std::optional<uint64_t> get_64bit_general_purpose_register(uint64_t register_id)
		{
			static std::map<uint64_t, uint64_t> table = []()
			{
				const std::map<uint64_t, std::set<uint64_t>> table_1 {
					{X86_REG_RAX, {X86_REG_RAX, X86_REG_EAX, X86_REG_AX, X86_REG_AL, X86_REG_AH}},
					{X86_REG_RBX, {X86_REG_RBX, X86_REG_EBX, X86_REG_BX, X86_REG_BL, X86_REG_BH}},
					{X86_REG_RCX, {X86_REG_RCX, X86_REG_ECX, X86_REG_CX, X86_REG_CL, X86_REG_CH}},
					{X86_REG_RDX, {X86_REG_RDX, X86_REG_EDX, X86_REG_DX, X86_REG_DL, X86_REG_DH}},
					{X86_REG_RSI, {X86_REG_RSI, X86_REG_ESI, X86_REG_SI, X86_REG_SIL}},
					{X86_REG_RDI, {X86_REG_RDI, X86_REG_EDI, X86_REG_DI, X86_REG_DIL}},
					{X86_REG_RBP, {X86_REG_RBP, X86_REG_EBP, X86_REG_BP, X86_REG_BPL}},
					{X86_REG_RSP, {X86_REG_RSP, X86_REG_ESP, X86_REG_SP, X86_REG_SPL}},
					{X86_REG_R8, {X86_REG_R8, X86_REG_R8D, X86_REG_R8W, X86_REG_R8B}},
					{X86_REG_R9, {X86_REG_R9, X86_REG_R9D, X86_REG_R9W, X86_REG_R9B}},
					{X86_REG_R10, {X86_REG_R10, X86_REG_R10D, X86_REG_R10W, X86_REG_R10B}},
					{X86_REG_R11, {X86_REG_R11, X86_REG_R11D, X86_REG_R11W, X86_REG_R11B}},
					{X86_REG_R12, {X86_REG_R12, X86_REG_R12D, X86_REG_R12W, X86_REG_R12B}},
					{X86_REG_R13, {X86_REG_R13, X86_REG_R13D, X86_REG_R13W, X86_REG_R13B}},
					{X86_REG_R14, {X86_REG_R14, X86_REG_R14D, X86_REG_R14W, X86_REG_R14B}},
					{X86_REG_R15, {X86_REG_R15, X86_REG_R15D, X86_REG_R15W, X86_REG_R15B}},
				};

				//通过上面的表，构建反向表
				std::map<uint64_t, uint64_t> table_2;
				for(const auto & one : table_1)
				{
					for(const auto & id : one.second)
						utils::map(table_2).insert_not_exist(id, one.first);
				}
				return table_2;
			}();

			if(utils::map(table).is_exist(register_id))
				return utils::map(table).get_existed_value(register_id);
			else
				return {};
		}
	}

	//把值复制过来，这样使用时，不用资源释放
	struct s_instruction_duplication
	{
	public:
		//复制过来的值
		cs_insn m_insn{};
		std::optional<cs_detail> m_detail;

	public:
		//比较函数
		bool operator==(const s_instruction_duplication &) const = default;
	};

	//对capstone的简单包装, 处理资源释放
	class Engine
	{
	public:
		struct s_instructions
		{
			cs_insn* insn{};
			size_t count{};
			uint64_t used_size{};
			
			//禁止简单复制
			s_instructions(const s_instructions &) = delete;
			s_instructions& operator=(const s_instructions &) = delete;
			s_instructions& operator=(s_instructions && o) = delete;
			s_instructions(s_instructions && o)
			{
				insn = o.insn;
				count = o.count;
				used_size = o.used_size;
				
				o.insn = 0;
				o.count = 0;
				o.used_size = 0;
			}

			s_instructions(){}
			~s_instructions()
			{
				if(count > 0) cs_free(insn, count);
			}
		};
	public:
		csh m_handle{};
	public:
		Engine(bool is_detail = false)
		{
			utils_assert(CS_ERR_OK == cs_open(CS_ARCH_X86, CS_MODE_64, &m_handle));

			if(is_detail) utils_assert(CS_ERR_OK == cs_option(m_handle, CS_OPT_DETAIL, CS_OPT_ON));
		}
		~Engine()
		{
			utils_assert_abort(cs_close(&m_handle) == CS_ERR_OK);
		}
		//对cs_disasm()的简单包装
		s_instructions disasm(const uint8_t* code, size_t code_size, uint64_t address, size_t count)
		{
			s_instructions result{};
			result.count = ::cs_disasm(m_handle, code, code_size, address, count, &result.insn);
			if (result.count == 0)
			{
				utils_assert(cs_errno(m_handle) == CS_ERR_OK);
			}
			for(uint64_t j = 0; j < result.count; j++)
			{
				result.used_size += result.insn[j].size;
			}
			return result;
		}
		static s_instruction_duplication get_instruction_duplication(const cs_insn & insn)
		{
			//复制本身
			s_instruction_duplication duplication;
			duplication.m_insn = insn;
			
			//复制detail
			if(insn.detail)
			{
				//防止被无意使用
				duplication.m_insn.detail = nullptr;

				duplication.m_detail = *insn.detail;
			}

			//给出
			return duplication;
		}
		//对cs_regs_access进行包装
		std::set<uint64_t> cs_regs_access(const s_instruction_duplication & instruction, uint64_t access)
		{
			//简单检查
			utils_assert(instruction.m_detail);
			utils_assert(access & (CS_AC_READ | CS_AC_WRITE));

			//构造一个insn，专门为接下来的函数调用来使用
			cs_insn insn{};
			insn = instruction.m_insn;
			insn.detail = const_cast<cs_detail*>(&instruction.m_detail.value());
			
			cs_regs regs_read{};
			uint8_t regs_read_count{};
			cs_regs regs_write{};
			uint8_t regs_write_count{};
			utils_assert(CS_ERR_OK == ::cs_regs_access(m_handle, &insn, regs_read,
				&regs_read_count, regs_write, &regs_write_count));

			std::set<uint64_t> used_regs;
			if(access & CS_AC_READ)
			{
				for(uint64_t i = 0; i < regs_read_count; i++)
					used_regs.insert(regs_read[i]);
			}
			if(access & CS_AC_WRITE)
			{
				for(uint64_t i = 0; i < regs_write_count; i++)
					used_regs.insert(regs_write[i]);
			}

			return used_regs;
		}
	};

	inline Engine & get_static_engine(bool is_detail = true)
	{
		//todo 看看是否需要变为，支持各个架构和is_detail真假两种情况

		//简单检查
		utils_assert(is_detail);

		static Engine engine{is_detail};
		return engine;
	}
	inline std::u8string get_register_name(uint64_t register_id)
	{
		static Engine engine;
		const auto text = cs_reg_name(engine.m_handle, register_id);
		utils_assert(text);
		return (const char8_t*)text;
	}
	class RegisterUseManager
	{
		//请指定
	public:
		const s_instruction_duplication* m_instruction{};
		std::function<void(uint64_t func_entry, bool & is_known, uint64_t & param_count)> m_get_function_param_count;

	public:
		//查询所有使用到的寄存器
		std::set<uint64_t> get_used_registers(uint64_t access)
		{
			//简单检查
			utils_assert(m_instruction);
			utils_assert(m_instruction->m_detail);

			//查询用到的寄存器, 在一般情况时
			std::set<uint64_t> used_registers = get_static_engine().cs_regs_access(*m_instruction, access);
			
			//特殊情况: 如果是一次函数调用，则会使用一些寄存器
			if (m_instruction->m_insn.id == X86_INS_CALL)
			{
				if(access & CS_AC_READ)
				{
					//查询被调函数的参数个数
					const auto param_count = [&]() ->uint64_t
					{
						//无法查询
						if(!m_get_function_param_count) return 4;

						//简单检查
						utils_assert(m_instruction->m_detail->x86.op_count == 1);
						utils_assert(m_instruction->m_detail->x86.operands[0].type == X86_OP_IMM);
						utils_assert(m_instruction->m_detail->x86.operands[0].size == 8);

						//被调函数
						const auto func_entry = (uint64_t)m_instruction->m_detail->x86.operands[0].imm;

						//查询
						bool is_known{};
						uint64_t param_count{};
						m_get_function_param_count(func_entry, is_known, param_count);
						
						//无法查询
						if(!is_known) return 4;

						//给出
						return param_count;
					}();

					if(param_count >= 1) used_registers.insert(X86_REG_RCX);
					if(param_count >= 2) used_registers.insert(X86_REG_RDX);
					if(param_count >= 3) used_registers.insert(X86_REG_R8);
					if(param_count >= 4) used_registers.insert(X86_REG_R9);
				}
				if(access & CS_AC_WRITE)
				{
					used_registers.insert(X86_REG_RAX);
					//used_registers.insert(X86_REG_RCX);	//todo 看看这里怎么写
					//used_registers.insert(X86_REG_RDX);
					//used_registers.insert(X86_REG_R8);
					//used_registers.insert(X86_REG_R9);
					used_registers.insert(X86_REG_R10);
					used_registers.insert(X86_REG_R11);
				}
			}
			//特殊情况: 如果是xor，并且操作数是相同的寄存器, 则视为写入，而不是读写
			if (m_instruction->m_insn.id == X86_INS_XOR && (access & CS_AC_READ))
			{
				if (m_instruction->m_detail->x86.op_count == 2 &&
					m_instruction->m_detail->x86.operands[0].type == X86_OP_REG &&
					m_instruction->m_detail->x86.operands[1].type == X86_OP_REG &&
					m_instruction->m_detail->x86.operands[0].reg ==
					m_instruction->m_detail->x86.operands[1].reg)
				{
					const auto reg = m_instruction->m_detail->x86.operands[0].reg;
					utils::set(used_registers).remove(reg);
				}
			}

			//给出
			return used_registers;
		}
		//查询是否使用了指定的64位寄存器
		//64位寄存器: 只统计r0-r15 这16个寄存器
		//            对于小于64位的，比如ax，保存为对应的64位，比如rax
		//            对于等于64位的，比如rax，保存为他自身，比如rax
		bool is_use_this_64bit_general_purpose_register(uint64_t register_id, uint64_t access)
		{
			//简单检查
			utils_assert(utils::set(register_id::get_all_64bit_general_purpose_register()).is_exist(register_id));

			//查询所有使用的寄存器
			const auto regs = get_used_registers(access);

			//对于每个寄存器，对应到64位寄存器，检查是否为指定的寄存器
			for(const auto reg : regs)
			{
				//变为相应的64位寄存器
				const auto reg_64 = register_id::get_64bit_general_purpose_register(reg);

				//使用了指定的寄存器
				if(reg_64 == register_id)
					return true;
			}

			//没有使用
			return false;
		}
	};
}