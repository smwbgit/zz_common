#pragma once

#include <windows.h>
#include <vector>
#include <utils/data.hpp>
#include <utils/string.hpp>
#include <utils/text/hex.hpp>

#if 0
namespace utils::windows::hook::hotpatch
{
	//热补丁
	//note: 目标函数的开头需要是固定的那一个指令
	//note: 目标函数的末尾+1需要是固定的那一个指令
	//note: 目标函数里的所有指令，需要是位置无关的
	//note: 析构时会还原所做的修改
#if 0
	example:

	std::unique_ptr<utils::windows::hook::hotpatch::HotPatch64> g_hotpatch_GetCursorPos;

	BOOL WINAPI my_GetCursorPos(_Out_ LPPOINT lpPoint)
	{
		if (0)
		{
			return TRUE;
		}
		else
		{
			using func_t = BOOL(WINAPI*)(_Out_ LPPOINT lpPoint);
			return ((func_t)g_hotpatch_GetCursorPos->m_old_func)(lpPoint);
		}
	}

	void work()
	{
		g_hotpatch_GetCursorPos = std::make_unique<utils::windows::hotpatch::HotPatch64>(
			0x00007FFD91A21570, 0x00007FFD91A21588 - 0x00007FFD91A21570, my_GetCursorPos);

		//到此完成补丁操作
		...
	}
#endif
	class HotPatch64
	{
	private:
		struct s_old_memory
		{
			uint64_t addr{};
			std::vector<uint8_t> data;
		};
	public:
		void* m_old_func{};
	private:
		inline static const std::vector<uint8_t> m_first{0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00};	//第一条指令
		std::vector<s_old_memory> m_old_memorys;	//还原时使用
	public:
		//begin: 目标函数的起始地址
		//size: 目标函数的大小
		//my_func: 接管函数
		HotPatch64(uint64_t begin, uint64_t size, void* my_func)
		{
			//简单检查
			utils_assert(size > 0);

			//准备数据
			uint64_t end = begin + size; //目标函数的末尾地址+1

			//检查是否符合格式
			utils_assert(memcmp((void*)(begin - m_first.size()), m_first.data(), m_first.size()) == 0);
			utils_assert(memcmp((void*)end, m_first.data(), m_first.size()) == 0);

			//保存现有
			m_old_func = VirtualAlloc(0, end - begin, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			utils_assert(m_old_func);
			memmove(m_old_func, (void*)begin, end - begin);

			//修改目标函数
			set_memory(begin - m_first.size(), {0xff, 0x25, 0x1a, 0, 0, 0});
			set_memory(end, (uint64_t)my_func);
			set_memory(begin, {0xeb, 0xf6});
		}
		~HotPatch64()
		{
			//todo 检查 目标函数的代码 是否和预期一样

			//还原目标函数
			for (const auto & one : m_old_memorys)
				set_memory(one.addr, one.data, false);
		}
	private:
		void set_memory(uint64_t addr, const std::vector<uint8_t> & data, bool is_save = true)
		{
			//简单检查
			utils_assert(!data.empty());

			//保存现有
			if (is_save)
			{
				std::vector<uint8_t> old;
				old.assign((uint8_t*)addr, (uint8_t*)(addr + data.size()));
				m_old_memorys.push_back({addr, old});
			}

			//修改
			DWORD old{};
			utils_assert(VirtualProtect((LPVOID)addr, data.size(), PAGE_EXECUTE_READWRITE, &old));
			memmove((BYTE*)addr, data.data(), data.size());
			DWORD old2{};
			utils_assert(VirtualProtect((LPVOID)addr, data.size(), old, &old2));
		}
		//特化
		void set_memory(uint64_t addr, uint64_t value, bool is_save = true)
		{
			//把uint64_t拆成8个字符
			auto _value = value;
			std::vector<uint8_t> data;
			for (int i = 0; i < 8; i++)
			{
				data.push_back(_value & 0xff);
				_value >>= 8;
			}

			//转发
			return set_memory(addr, data, is_save);
		}
	};

	//帮助找到函数体的起始位置
	//example:
	//	g_hotpatch_GetCursorPos = std::make_unique<utils::windows::hotpatch::HotPatch64>(
	//		utils::windows::hotpatch::InstructionWalker{GetCursorPos}.shift(0x9).follow_jmp({0x48,0xFF,0x25}).get_addr(), 0x18,
	//		my_GetCursorPos);
	class InstructionWalker
	{
	private:
		uint64_t m_addr{};
	public:
		InstructionWalker(void* start_addr)
			: m_addr{(uint64_t)start_addr}
		{
		}
		//特化
		InstructionWalker(uint64_t start_addr)
			: m_addr{start_addr}
		{
		}
		InstructionWalker& shift(int64_t offset)
		{
			m_addr += offset;
			return *this;
		}
		InstructionWalker& follow_jmp(const std::vector<uint8_t> & opcode)
		{
			if (opcode == std::vector<uint8_t>{0x48, 0xFF, 0x25})
			{
				//00007FFD938B7DA9 48 FF 25 C0 9F 06 00 jmp         qword ptr[7FFD93921D70h]

				//简单检查
				utils_assert(memcmp((const void*)m_addr, opcode.data(), opcode.size()) == 0);

				auto p = m_addr + 7 + *(uint32_t*)(m_addr + 3);
				m_addr = *(uint64_t*)p;
			}
			else if (opcode == std::vector<uint8_t>{0xFF, 0x25})
			{
				//00007FFD938C4BB0 FF 25 82 D7 05 00    jmp         qword ptr[7FFD93922338h]

				//简单检查
				utils_assert(memcmp((const void*)m_addr, opcode.data(), opcode.size()) == 0);

				auto p = m_addr + 6 + *(uint32_t*)(m_addr + 2);
				m_addr = *(uint64_t*)p;
			}
			else
				utils_throw();

			return *this;
		}
		uint64_t get_addr() const
		{
			return m_addr;
		}
	};

}
#endif

namespace utils::windows::hook::hotpatch
{
	//对内存做修改和还原操作
	class MemoryPatcher
	{
	private:
		struct s_old_memory
		{
			uint64_t addr{};
			utils::data data;
		};
	private:
		std::vector<s_old_memory> m_old_memorys;	//还原时使用
	public:
		~MemoryPatcher()
		{
			undo();
		}
		void set_memory(uint64_t addr, const utils::data & data, bool is_save = true)
		{
			//简单检查
			utils_assert(!data.empty());

			//保存现有
			if (is_save)
			{
				utils::data old;
				old.assign((uint8_t*)addr, data.size());
				m_old_memorys.push_back({addr, std::move(old)});
			}

			//修改
			DWORD old{};
			utils_assert(VirtualProtect((LPVOID)addr, data.size(), PAGE_EXECUTE_READWRITE, &old));
			if(data.size() == 2)	//期望复制能是原子的	//todo 看看这里这么写是不是原子的
				*(uint16_t*)addr = *(uint16_t*)data.addr();
			else
				memmove((BYTE*)addr, data.addr(), data.size());
			DWORD old2{};
			utils_assert(VirtualProtect((LPVOID)addr, data.size(), old, &old2));
		}
		//特化
		template<typename T, typename = std::enable_if_t<std::is_same_v<T,uint64_t> || std::is_same_v<T,uint32_t>>>
		void set_memory(uint64_t addr, T value, bool is_save = true)
		{
			utils::data data;
			data.append((uint8_t*)&value, sizeof(value));

			//转发
			return set_memory(addr, data, is_save);
		}
		//恢复原样
		void undo()
		{
			//todo 检查当前内存内容是否和预期一样

			//还原
			for (auto iter = m_old_memorys.rbegin(); iter != m_old_memorys.rend(); iter++)	//逆序
			{
				const auto & one = *iter;
				set_memory(one.addr, one.data, false);
			}
			m_old_memorys.clear();
		}
	};

	//专门负责维护dll间的，自己分配出来的，用于8字节跳转地址存放，的内存块
	class JumpTable
	{
	private:
		class s_table
		{
		private:
			uint64_t m_start{};
			uint64_t m_size{};	//已用的item的个数. [0, 0x1000/8]
			uint64_t m_max_size{0x1000 / 8};
		public:
			s_table(uint64_t jmp_i32__offset_begin)
			{
				[this, jmp_i32__offset_begin]()
				{
					uint64_t center = jmp_i32__offset_begin & 0xfffffffffffff000;
					for (int64_t i = -0x70000000; i < 0x70000000; i += 0x1000)
					{
						auto p = center + i;
						auto re = VirtualAlloc((void*)p, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
						if (re)
						{
							m_start = (uint64_t)re;
							return;
						}
					}
					utils_throw();
				}();
			}
			bool is_near_enough(uint64_t jmp_i32__offset_begin)
			{
				if (m_size >= m_max_size) return false;

				auto offset = (int64_t)jmp_i32__offset_begin - (int64_t)m_start;
				int32_t _offset{};
				return utils::computer_basic_function::try_store_in(_offset, offset);
			}
			int32_t add(uint64_t jmp_i32__offset_begin, uint64_t dest)
			{
				//简单检查
				utils_assert(m_size < m_max_size);

				uint64_t* item = (uint64_t*)m_start + m_size;	m_size++;

				*item = dest;
				return utils::computer_basic_function::store_in<int32_t>((int64_t)item - (int64_t)jmp_i32__offset_begin);
			}
			s_table(const s_table &) = delete;
			s_table(s_table &&) = delete;
			~s_table()
			{
				utils_assert_abort(VirtualFree((void*)m_start, 0, MEM_RELEASE));
			}
		};
		std::vector<std::unique_ptr<s_table>> m_tables;
	public:
		//return: jmp_i32__offset
		int32_t add(uint64_t jmp_i32__offset_begin, uint64_t dest)
		{
			//找到一个足够近的表
			auto & table = [this, jmp_i32__offset_begin]() -> s_table &
			{
				for (auto & one : m_tables)
					if (one->is_near_enough(jmp_i32__offset_begin))
						return *one;

				//创建
				m_tables.push_back(std::make_unique<s_table>(jmp_i32__offset_begin));
				return *m_tables[m_tables.size() - 1];
			}();

			//添加
			return table.add(jmp_i32__offset_begin, dest);
		}
	};

	//热补丁
	class HotPatch64
	{
	public:
		struct ReturnValue
		{
			inline static const uint64_t call_org_func = 0x1234567890bbcd00;
		};
	private:
		MemoryPatcher m_memory_patcher;
		LPVOID m_hook_func{};
	public:
		//begin: 目标函数的起始地址
		//before: 目标函数前，6个字节的内容
		//first: 目标函数第一条指令的内容
		//my_func: 接管函数，返回0x1234567890bbcd00表示调用原函数
		HotPatch64(void* hooked_func,
			std::vector<std::string> vec_str_before,
			std::vector<std::string> vec_str_first,
			JumpTable & jump_table,
			void* my_func)
		{
			//简单转换
			auto get_vec_data = [](const std::vector<std::string> & vec_str)
			{
				std::vector<utils::data> vec;
				for (const auto & one : vec_str)
					vec.push_back(utils::text::hex::from_text(one, true));
				return vec;
			};
			std::vector<utils::data> vec_before = get_vec_data(vec_str_before);
			std::vector<utils::data> vec_first = get_vec_data(vec_str_first);
			uint64_t begin = (uint64_t)hooked_func;

			//简单检查
			for(const auto & before : vec_before)
				utils_assert(before.size() == 6);
			for (const auto & first : vec_first)
				utils_assert(first.size() >= 2);

			//简单检查
			auto find_which = [](const std::vector<utils::data> & vec, uint64_t addr) -> const utils::data &
			{
				for (const auto & one : vec)
					if (memcmp((void*)addr, one.addr(), one.size()) == 0)
						return one;
				utils_throw();
			};
			const auto & before = find_which(vec_before, begin - 6);
			const auto & first = find_which(vec_first, begin);

			//处理第一条指令
			auto _first = first;
			auto _second_inst = begin+first.size();
			if(first.size() == 6 && first[0] == 0xFF && first[1] == 0x25)
			{
				//直接跳转到原来第一条指令想跳转到的地方
				
				auto p = begin + 6 + *(int32_t*)&first[2];
				_second_inst = *(uint64_t*)p;

				_first.clear();
			}
			else if(first.size() == 7 && first[0] == 0x48 && first[1] == 0xFF && first[2] == 0x25)
			{
				//直接跳转到原来第一条指令想跳转到的地方
				
				auto p = begin + 7 + *(int32_t*)&first[3];
				_second_inst = *(uint64_t*)p;

				_first.clear();
			}

			//制作hook函数
			auto hook_func_opcode = prepare_hook_func(_first, my_func, _second_inst);
			m_hook_func = VirtualAlloc(0, hook_func_opcode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			utils_assert(m_hook_func);
			memmove(m_hook_func, hook_func_opcode.addr(), hook_func_opcode.size());

			//填写跳转表
			auto last_offset = jump_table.add(begin, (uint64_t)m_hook_func);

			//修改目标函数
			m_memory_patcher.set_memory(begin - before.size(), std::vector<uint8_t>{0xff, 0x25});
			m_memory_patcher.set_memory(begin - before.size()+2, (uint32_t)last_offset);
			m_memory_patcher.set_memory(begin, std::vector<uint8_t>{0xeb, 0xf8});
		}
		//恢复入口指令
		//note:主要提供给函数执行时间长的情况，此时，可以先调用这个函数，sleep几秒，再进行析构，以防止rip还在my_func的函数体里
		void undo_entry()
		{
			m_memory_patcher.undo();
		}
		~HotPatch64()
		{
			if(m_hook_func)
				utils_assert_abort(VirtualFree(m_hook_func, 0, MEM_RELEASE));
		}
	private:
		utils::data prepare_hook_func(const utils::data & org_func_first_inst, void* my_func, uint64_t org_func_second_inst)
		{
#if 0
.data

.code

;note: 如果my_func返回1234567890bbcd00H，则不再调用原函数，直接返回
hotpatch_func_head proc
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push rbp
	pushfq

	sub rsp,20H
	call [my_func]
	add rsp,20H
	mov rcx,1234567890bbcd00H
	cmp rax,rcx
	je call_org
	
;direct_return:
	popfq
	pop rbp
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	add rsp,8	;直接使用my_func的返回值
	ret

call_org:
	popfq
	pop rbp
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	org_func_first_inst_placehold	dq 1234567890abcd00H
	jmp [org_func_second_inst]

;data:
	my_func 						dq 1234567890abcd01H
	org_func_second_inst 			dq 1234567890abcd02H
hotpatch_func_head endp

end
#endif
			utils::string opcode = R"(
				50 53 51 52 56 57 41 50 41 51 41 52 41 53 41 54  
				41 55 41 56 41 57 55 9c 48 83 ec 20 ff 15 55 00  
				00 00 48 83 c4 20 48 b9 00 cd bb 90 78 56 34 12  
				48 3b c1 74 1c 9d 5d 41 5f 41 5e 41 5d 41 5c 41  
				5b 41 5a 41 59 41 58 5f 5e 5a 59 5b 48 83 c4 08  
				c3 9d 5d 41 5f 41 5e 41 5d 41 5c 41 5b 41 5a 41  
				59 41 58 5f 5e 5a 59 5b 58 00 cd ab 90 78 56 34  
				12 ff 25 08 00 00 00 01 cd ab 90 78 56 34 12 02  
				cd ab 90 78 56 34 12 
			)";
			utils::data _opcode;
			utils::text::hex::from_text(_opcode, opcode, true);
			_opcode.replace_unique((uint64_t)0x1234567890abcd00, [&org_func_first_inst]()
			{
				//简单检查
				utils_assert(org_func_first_inst.size() >= 0 && org_func_first_inst.size() <= 8);

				auto inst = org_func_first_inst;
				inst.wrapped_type().resize(8, 0x90);    //剩余的地方用nop补齐
				return *(uint64_t*)inst.addr();
			}());
			_opcode.replace_unique((uint64_t)0x1234567890abcd01, (uint64_t)my_func);
			_opcode.replace_unique((uint64_t)0x1234567890abcd02, org_func_second_inst);
			return _opcode;
		}
	};

	//帮助找到函数体的起始位置
	//example:
	//	g_hotpatch_GetCursorPos = std::make_unique<utils::windows::hotpatch::HotPatch64>(
	//		utils::windows::hotpatch::InstructionWalker{GetCursorPos}.shift(0x9).follow_jmp({0x48,0xFF,0x25}).get_addr(), 0x18,
	//		my_GetCursorPos);
	class InstructionWalker
	{
	private:
		uint64_t m_addr{};
	public:
		InstructionWalker(void* start_addr)
			: m_addr{(uint64_t)start_addr}
		{
		}
		//特化
		InstructionWalker(uint64_t start_addr)
			: m_addr{start_addr}
		{
		}
		InstructionWalker& shift(int64_t offset)
		{
			m_addr += offset;
			return *this;
		}
		InstructionWalker& follow_jmp(const std::vector<uint8_t> & opcode)
		{
			if (opcode == std::vector<uint8_t>{0x48, 0xFF, 0x25})
			{
				//00007FFD938B7DA9 48 FF 25 C0 9F 06 00 jmp         qword ptr[7FFD93921D70h]

				//简单检查
				utils_assert(memcmp((const void*)m_addr, opcode.data(), opcode.size()) == 0);

				auto p = m_addr + 7 + *(uint32_t*)(m_addr + 3);
				m_addr = *(uint64_t*)p;
			}
			else if (opcode == std::vector<uint8_t>{0xFF, 0x25})
			{
				//00007FFD938C4BB0 FF 25 82 D7 05 00    jmp         qword ptr[7FFD93922338h]

				//简单检查
				utils_assert(memcmp((const void*)m_addr, opcode.data(), opcode.size()) == 0);

				auto p = m_addr + 6 + *(uint32_t*)(m_addr + 2);
				m_addr = *(uint64_t*)p;
			}
			else
				utils_throw();

			return *this;
		}
		uint64_t get_addr() const
		{
			return m_addr;
		}
	};

}
