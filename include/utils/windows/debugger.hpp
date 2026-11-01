#pragma once

#include <windows.h>
#include <winternl.h>
#include <utils/macro.hpp>
#include <utils/assert.hpp>
#include <utils/map.hpp>
#include <utils/CallWhenProgramStart.hpp>
#include <utils/windows/memory.hpp>
#include <utils/windows/advanced/wnf.hpp>

namespace utils::windows::debugger
{
	//设置为单步执行
	inline void set_single_step(HANDLE hThread)
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		utils_assert(GetThreadContext(hThread, &context));
			
		// 设置EFLAGS中的TF标志，以启用单步执行
		context.EFlags |= 0x100; // 0x100是TF标志的位掩码
			
		// 设置修改后的上下文
		utils_assert(SetThreadContext(hThread, &context));
	}
	//修改rip:偏移
	//shift:偏移量
	inline void shift_rip(HANDLE hThread, int64_t shift)
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		utils_assert(GetThreadContext(hThread, &context));
			
		context.Rip = (int64_t)context.Rip + shift;
			
		// 设置修改后的上下文
		utils_assert(SetThreadContext(hThread, &context));
	}
	//修改rip
	inline void set_rip(HANDLE hThread, uint64_t rip)
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		utils_assert(GetThreadContext(hThread, &context));
			
		context.Rip = rip;
			
		// 设置修改后的上下文
		utils_assert(SetThreadContext(hThread, &context));
	}
}
namespace utils::windows::debugger::call_stack
{
	//获得可能的调用栈
	//note: 根据是否在一个可执行的页面上 以及 它的前面是否有个call指令
	inline std::vector<uint64_t> get_potential_call_stack(HANDLE hProcess, HANDLE hThread)
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		utils_assert(GetThreadContext(hThread, &context));

		//简单检查
		utils_assert(context.Rsp);
		utils_assert((context.Rsp & 0x7) == 0);

		auto mbi = utils::windows::memory::query(hProcess, context.Rsp);

		//简单检查
		utils_assert(mbi.has_value() && mbi->RegionSize >= 0x1000);
		utils_assert((uint64_t)context.Rsp <= (uint64_t)mbi->BaseAddress + (uint64_t)mbi->RegionSize);

		//栈大小
		const auto size = (uint64_t)mbi->BaseAddress + (uint64_t)mbi->RegionSize - (uint64_t)context.Rsp;

		//简单检查
		utils_assert(size % 8 == 0);
		
		//读取栈
		std::vector<uint64_t> stack;
		stack.resize(size/8);
		utils::windows::memory::read_memory(hProcess, context.Rsp, stack.data(), size);

		std::vector<uint64_t> potential_call_stack;
		potential_call_stack.push_back(context.Rip);	//把当前的函数也放里
		for(const auto value : stack)
		{
			if (utils::windows::memory::is_executable(hProcess, value))
			{
				if ([&]()
				{
					return true;

					//todo 待实现
					if (utils::windows::memory::is_executable(hProcess, value-5))
					{
						uint8_t c{};
						utils::windows::memory::read_memory(hProcess, value-5, &c, 1);
						if (c == 0xe8)
							return true;
					}
				}())
					potential_call_stack.push_back(value);
			}
		}
		return potential_call_stack;
	}
#if 0
	//特化: 根据unwind信息
	inline std::vector<uint64_t> get_potential_call_stack_by_unwind(HANDLE hProcess, HANDLE hThread)
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		utils_assert(GetThreadContext(hThread, &context));

		//简单检查
		utils_assert(context.Rsp);
		utils_assert((context.Rsp & 0x7) == 0);

		auto mbi = utils::windows::memory::query(hProcess, context.Rsp);

		//简单检查
		utils_assert(mbi.has_value() && mbi->RegionSize >= 0x1000);
		utils_assert((uint64_t)context.Rsp <= (uint64_t)mbi->BaseAddress + (uint64_t)mbi->RegionSize);

		//栈大小
		const auto size = (uint64_t)mbi->BaseAddress + (uint64_t)mbi->RegionSize - (uint64_t)context.Rsp;

		//简单检查
		utils_assert(size % 8 == 0);
		
		//读取栈
		std::vector<uint64_t> stack;
		stack.resize(size/8);
		utils::windows::memory::read_memory(hProcess, context.Rsp, stack.data(), size);

		std::vector<uint64_t> potential_call_stack;
		potential_call_stack.push_back(context.Rip);	//把当前的函数也放里
		for(const auto value : stack)
		{
			if (utils::windows::memory::is_executable(hProcess, value))
			{
				if ([&]()
				{
					return true;

					//todo 待实现
					if (utils::windows::memory::is_executable(hProcess, value-5))
					{
						uint8_t c{};
						utils::windows::memory::read_memory(hProcess, value-5, &c, 1);
						if (c == 0xe8)
							return true;
					}
				}())
					potential_call_stack.push_back(value);
			}
		}
		return potential_call_stack;
	}
#endif
}
namespace utils::windows::debugger::to_text
{
	inline std::u8string Size(uint32_t value)
	{
		return utils::text::format::format(u8"0x%x", value);
	}
	inline std::u8string Size(uint64_t value)
	{
		return utils::text::format::format(u8"0x%llx", value);
	}
	inline std::u8string HANDLE(uint64_t value)
	{
		return utils::text::format::format(u8"0x%llx", value);
	}
	inline std::u8string NTSTATUS(uint32_t value)
	{
		return utils::text::format::format(u8"0x%x", value);
	}
	inline std::u8string Addr(uint64_t value)
	{
		return utils::text::format::format(u8"0x%llx", value);
	}
	inline std::u8string TimeStamp(uint32_t value)
	{
		return utils::text::format::format(u8"0x%x", value);
	}
}
namespace utils::windows::debugger::to_text::flag
{
	//从一个位组合中读出各个flag
	template<typename T>
	class Bits
	{
	private:
		T m_flag{};
		std::u8string m_text;
	public:
		Bits(T flag)
			: m_flag{flag}
		{
		}
		void test(T flag, const std::u8string & name)
		{
			if (m_flag & flag)
			{
				if(m_text.empty())
					m_text += name;
				else
					m_text += u8"|" + name;

				//去掉此flag
				m_flag &= ~flag;
			}
		}
		std::u8string get_text()
		{
			//简单检查: 已没有未识别到的
			utils_assert(m_flag == 0);
				
			if(m_text.empty()) m_text = u8"none";

			return m_text;
		}
	};
	//从一个顺序表示的序列的flag中读出是哪个flag
	template<typename T>
	class Number
	{
	private:
		T m_value{};
		std::optional<std::u8string> m_text;
	public:
		Number(T value)
			: m_value{value}
		{
		}
		void test(T value, const std::u8string & name)
		{
			if (m_value == value)
			{
				//简单检查
				utils_assert(!m_text.has_value());

				m_text = name;
			}
		}
		std::u8string get_text()
		{
			//简单检查: 已没有未识别到的
			utils_assert(m_text.has_value());

			return m_text.value();
		}
	};
}
namespace utils::windows::debugger::to_text::nt_type
{
	inline std::u8string EVENT_TYPE(uint32_t value)
	{
		if(value == 0) return u8"NotificationEvent";
		if(value == 1) return u8"SynchronizationEvent";
		utils_throw();
	}
	inline std::u8string BOOLEAN(uint8_t value)
	{
		utils_assert(value == 0 || value == 1);

		return value ? u8"TRUE" : u8"FALSE";
	}
	inline std::u8string ACCESS_MASK(uint32_t flag, const std::u8string & type)
	{
		//简单检查
		utils_assert(sizeof(::ACCESS_MASK) == 4);
				
		utils::windows::debugger::to_text::flag::Bits<uint32_t> text{flag};
				
		//通用权限
		text.test(utils_macro__A_and_textA(GENERIC_READ));
		text.test(utils_macro__A_and_textA(GENERIC_WRITE));
		text.test(utils_macro__A_and_textA(GENERIC_EXECUTE));
		text.test(utils_macro__A_and_textA(GENERIC_ALL));
		text.test(utils_macro__A_and_textA(DELETE));
		text.test(utils_macro__A_and_textA(READ_CONTROL));
		text.test(utils_macro__A_and_textA(WRITE_DAC));
		text.test(utils_macro__A_and_textA(WRITE_OWNER));
		text.test(utils_macro__A_and_textA(SYNCHRONIZE));
				
		//特定于对象的权限

		if(type == u8"file")
		{
			text.test(utils_macro__A_and_textA(FILE_READ_DATA));
			text.test(utils_macro__A_and_textA(FILE_WRITE_DATA));
			text.test(utils_macro__A_and_textA(FILE_APPEND_DATA));
			text.test(utils_macro__A_and_textA(FILE_READ_EA));
			text.test(utils_macro__A_and_textA(FILE_WRITE_EA));
			text.test(utils_macro__A_and_textA(FILE_EXECUTE));
			text.test(utils_macro__A_and_textA(FILE_READ_ATTRIBUTES));
			text.test(utils_macro__A_and_textA(FILE_WRITE_ATTRIBUTES));
		}
		else if (type == u8"directory")
		{
			text.test(utils_macro__A_and_textA(FILE_LIST_DIRECTORY));
			text.test(utils_macro__A_and_textA(FILE_TRAVERSE));
		}
		else if (type == u8"event")
		{
			text.test(0x0001, u8"EVENT_QUERY_STATE");
			text.test(0x0002, u8"EVENT_MODIFY_STATE");
		}
		else if (type == u8"io_completion_port")
		{
			text.test(0x0001, u8"IO_COMPLETION_QUERY_STATE");
			text.test(0x0002, u8"IO_COMPLETION_MODIFY_STATE");
		}
		else if (type == u8"unknown")
		{
			text.test(0x0001, u8"0x0001");
			text.test(0x0002, u8"0x0002");
			text.test(0x0004, u8"0x0004");
			text.test(0x0008, u8"0x0008");
			text.test(0x0010, u8"0x0010");
			text.test(0x0020, u8"0x0020");
			text.test(0x0040, u8"0x0040");
			text.test(0x0080, u8"0x0080");
			text.test(0x0100, u8"0x0100");
			text.test(0x0200, u8"0x0200");
			text.test(0x0400, u8"0x0400");
			text.test(0x0800, u8"0x0800");
			text.test(0x1000, u8"0x1000");
			text.test(0x2000, u8"0x2000");
			text.test(0x4000, u8"0x4000");
			text.test(0x8000, u8"0x8000");
		}
		else
			utils_throw();

		return text.get_text();
	}
}
namespace utils::windows::debugger::struct_data
{
	//读取另一个进程中的一个结构体

	namespace inner
	{
		//命名空间中其他类的共同父类
		//todo 看看把这个类放在外头, 而不是inner的, 名字叫 StructReader
		class Basic
		{
		public:
			utils::data m_struct_data;
		protected:
			void read_struct(HANDLE hProcess, uint64_t addr, uint64_t size)
			{
				m_struct_data = utils::windows::memory::read_memory(hProcess, addr, size);
			}
			//特化: 根据结构体的第一个成员，来确定结构体的大小
			void read_struct_by_first_member(HANDLE hProcess, uint64_t addr, uint64_t first_member_size = 4)
			{
				if (first_member_size == 4)
				{
					uint32_t size{};
					utils::windows::memory::read_memory(hProcess, addr, &size, sizeof(size));

					return read_struct(hProcess, addr, size);
				}
				else
					utils_throw();
			}
		};
	}
	//可空指针
	template<typename T>
	class NullablePointer : public inner::Basic
	{
	public:
		bool is_nullptr{};
		T m_value{};
	public:
		template<typename...Args>
		void read(HANDLE hProcess, uint64_t addr, Args && ... args)
		{
			if (addr)
			{
				m_value.read(hProcess, addr, std::forward<Args>(args)...);
				is_nullptr = false;
			}
			else
			{
				is_nullptr = true;
			}
		}
		std::u8string to_text() const
		{
			if (is_nullptr)
			{
				return u8"nullptr";
			}
			else
			{
				return m_value.to_text();
			}
		}
	};
	//针对uint8_t*类型
	class NATIVE_DATA : public inner::Basic
	{
	public:
		void read(HANDLE hProcess, uint64_t addr, uint64_t size)
		{
			read_struct(hProcess, addr, size);
		}
		std::u8string to_text() const
		{
			return u8"unknown";
		}
	};
	//针对wchar_t*类型
	class NATIVE_UNICODE_STRING : public inner::Basic
	{
	public:
		std::u16string m_text;

	public:
		void read(HANDLE hProcess, uint64_t addr, uint64_t length)
		{
			read_struct(hProcess, addr, length*2);
			m_text.assign((char16_t*)m_struct_data.addr(), length);
		}
		std::u8string to_text() const
		{
			return u8"\"" + utils::text::encoding::unicode::utf16_to_utf8(m_text) + u8"\"";
		}
	};
	class GUID : public inner::Basic
	{
	public:
		void read(HANDLE hProcess, uint64_t addr)
		{
			read_struct(hProcess, addr, 16);
		}
		std::u8string to_text() const
		{
			utils::uuid uuid{};
			memmove(uuid.data, m_struct_data.addr(), 16);
			std::string uuid_text = boost::uuids::to_string(uuid);
			return (const char8_t*)uuid_text.c_str();
		}
	};
}
namespace utils::windows::debugger::struct_data::nt_struct
{
	namespace inner
	{
		using Basic = utils::windows::debugger::struct_data::inner::Basic;
		namespace struct_data = utils::windows::debugger::struct_data;
	}

	class UNICODE_STRING : public inner::Basic
	{
	public:
		USHORT m_Length{};
		USHORT m_MaximumLength{};
		inner::struct_data::NATIVE_UNICODE_STRING m_Buffer;

	public:
		void read(HANDLE hProcess, uint64_t addr)
		{
			read_struct(hProcess, addr, 0x10);

			m_Length = *(uint16_t*)&m_struct_data[0];
			m_MaximumLength = *(uint16_t*)&m_struct_data[2];

			//简单检查
			utils_assert(m_Length <= m_MaximumLength);
			utils_assert(m_Length % 2 == 0);
			utils_assert(m_MaximumLength % 2 == 0);

			m_Buffer.read(hProcess, *(uint64_t*)&m_struct_data[8], m_Length/2);
		}
		std::u8string to_text() const
		{
			return m_Buffer.to_text();
		}
	};
	class LARGE_INTEGER : public inner::Basic
	{
	public:
		LONGLONG m_QuadPart{};

	public:
		void read(HANDLE hProcess, uint64_t addr)
		{
			read_struct(hProcess, addr, 0x8);

			m_QuadPart = *(LONGLONG*)&m_struct_data[0];
		}
		std::u8string to_text() const
		{
			return utils::text::format::format(u8"0x%llx", m_QuadPart);
		}
	};
	class OBJECT_ATTRIBUTES : public inner::Basic
	{
	public:
		nt_struct::UNICODE_STRING m_ObjectName;
	public:
		void read(HANDLE hProcess, uint64_t addr)
		{
			read_struct_by_first_member(hProcess, addr);

			m_ObjectName.read(hProcess, *(uint64_t*)&m_struct_data[0x10]);
		}
		std::u8string to_text() const
		{
			return u8"{ObjectName=" + m_ObjectName.to_text() +u8"}";
		}
	};
};
namespace utils::windows::debugger::function_call
{
	class FunctionCallReader
	{
	public:
		std::vector<uint64_t> m_args;
		std::optional<uint64_t> m_return_value;
	public:
		//读寄存器和栈上的参数, 不包括二级内容(也就是指针的内容)
		//n: 参数个数
		void read_stack_args(HANDLE hProcess, HANDLE hThread, uint64_t n)
		{
			if (n == 0) return;
			
			CONTEXT context{};
			context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
			utils_assert(GetThreadContext(hThread, &context));

			m_args.resize(n);
			m_args[0] = context.Rcx; if(n==1) return;
			m_args[1] = context.Rdx; if(n==2) return;
			m_args[2] = context.R8;  if(n==3) return;
			m_args[3] = context.R9;  if(n==4) return;
				
			utils::windows::memory::read_memory(hProcess, context.Rsp+sizeof(uint64_t)*5,
				&m_args[4], sizeof(uint64_t)*(n-4));
		}
		//读rax
		void read_return_value(HANDLE hProcess, HANDLE hThread)
		{
			CONTEXT context{};
			context.ContextFlags = CONTEXT_INTEGER;
			utils_assert(GetThreadContext(hThread, &context));

			m_return_value = context.Rax;
		}
	};
}
namespace utils::windows::debugger::function_call::ntdll_syscall
{
	class NtdllNtXxxx;

	namespace inner
	{
		namespace nt_type = utils::windows::debugger::to_text::nt_type;
		namespace nt_struct = debugger::struct_data::nt_struct;
		namespace struct_data = debugger::struct_data;
		
		std::map<std::u8string, std::function<std::unique_ptr<NtdllNtXxxx>()>> all_syscall;

		//注册一个nt函数，以便于NtdllNtXxxx::create能生成
		template<typename T>
		inline void register_syscall(const std::u8string & func_name, T*)
		{
			utils::map(inner::all_syscall).insert_not_exist(func_name, [](){
				return std::make_unique<T>();
			});
		}
	}
	//此命名空间的公共基类
	class NtdllNtXxxx
	{
	public:
		utils::windows::debugger::function_call::FunctionCallReader m_reader;

	public:
		virtual void read_args(HANDLE hProcess, HANDLE hThread) = 0;
		virtual std::u8string get_text_for_args() const = 0;
		virtual void read_return_value(HANDLE hProcess, HANDLE hThread)
		{
			m_reader.read_return_value(hProcess, hThread);
		}
		virtual std::u8string get_text_for_return_value()
		{
			//简单检查
			utils_assert(m_reader.m_return_value.has_value());

			namespace to_text = utils::windows::debugger::to_text;
			return u8"ReturnValue=" + to_text::NTSTATUS((uint32_t)m_reader.m_return_value.value());
		}
		virtual std::u8string get_text_for_note() const
		{
			return {};
		}

	public:
		static std::unique_ptr<NtdllNtXxxx> create(const std::u8string & func_name)
		{
			if(utils::map(inner::all_syscall).is_exist(func_name))
			{
				auto & debug = utils::map(inner::all_syscall).get_existed_value(func_name);
				auto ptr = debug();
				return std::move(ptr);
			}
			else
				return {};
		}
	};
	class NtWriteFile : public NtdllNtXxxx
	{
	public:
		struct_data::NATIVE_DATA m_Buffer;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 9);

			m_Buffer.read(hProcess, m_reader.m_args[5], *(uint32_t*)&m_reader.m_args[6]);
		}
		std::u8string get_text_for_args() const override
		{
			return utils::text::format::format(
				u8"FileHandle=0x%llx,Buffer=\"%s\",Length=0x%x",
				m_reader.m_args[0],
				u8"unknown",
				*(uint32_t*)&m_reader.m_args[6]);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtWriteFile", (NtWriteFile*)nullptr);});
	};
	class NtCreateEvent : public NtdllNtXxxx
	{
	public:
		inner::struct_data::NullablePointer<inner::nt_struct::OBJECT_ATTRIBUTES> m_ObjectAttributes;

		struct s_return
		{
			uint64_t m_EventHandle{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 5);

			m_ObjectAttributes.read(hProcess, m_reader.m_args[2]);
		}
		std::u8string get_text_for_args() const override
		{
			return std::u8string{}
				+ u8"DesiredAccess=" + inner::nt_type::ACCESS_MASK((uint32_t)m_reader.m_args[1], u8"event")
				+ u8",ObjectAttributes=" + m_ObjectAttributes.to_text()
				+ u8",EventType=" + inner::nt_type::EVENT_TYPE((uint32_t)m_reader.m_args[3])
				+ u8",InitialState=" + inner::nt_type::BOOLEAN((uint8_t)m_reader.m_args[4]);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_return->m_EventHandle, sizeof(m_return->m_EventHandle));
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			return NtdllNtXxxx::get_text_for_return_value() + utils::text::format::format(
				u8",EventHandle=0x%llx",
				m_return->m_EventHandle);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtCreateEvent", (NtCreateEvent*)nullptr);});
	};
	class NtClose : public NtdllNtXxxx
	{
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 1);
		}
		std::u8string get_text_for_args() const override
		{
			return utils::text::format::format(
				u8"Handle=0x%llx",
				m_reader.m_args[0]);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtClose", (NtClose*)nullptr);});
	};
	class NtTestAlert : public NtdllNtXxxx
	{
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 0);
		}
		std::u8string get_text_for_args() const override
		{
			return {};
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtTestAlert", (NtTestAlert*)nullptr);});
	};
	class NtTraceControl : public NtdllNtXxxx
	{
	public:
		inner::struct_data::NATIVE_DATA m_InputBuffer;
		struct s_return
		{
			inner::struct_data::NATIVE_DATA m_OutputBuffer;
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 6);

			m_InputBuffer.read(hProcess, m_reader.m_args[1], *(uint32_t*)&m_reader.m_args[2]);

		}
		std::u8string get_text_for_args() const override
		{
			std::u8string FunctionCode = [&]()
			{
				//refer to: https://ntdoc.m417z.com/etwtracecontrolcode
				//refer to: https://ntdoc.m417z.com/nttracecontrol
				switch(m_reader.m_args[0])
				{
				case 1: return u8"EtwStartLoggerCode";		// maybe inout WMI_LOGGER_INFORMATION
				case 2: return u8"EtwStopLoggerCode";		// maybe inout WMI_LOGGER_INFORMATION
				case 3: return u8"EtwQueryLoggerCode";		// maybe inout WMI_LOGGER_INFORMATION
				case 4: return u8"EtwUpdateLoggerCode";		// maybe inout WMI_LOGGER_INFORMATION
				case 5: return u8"EtwFlushLoggerCode";		// maybe inout WMI_LOGGER_INFORMATION
				case 6: return u8"EtwIncrementLoggerFile";	// maybe inout WMI_LOGGER_INFORMATION
				case 7: return u8"EtwRealtimeTransition";	// maybe inout WMI_LOGGER_INFORMATION
				case 11: return u8"EtwRealtimeConnectCode";
				case 12: return u8"EtwActivityIdCreate";
				case 13: return u8"EtwWdiScenarioCode";
				case 14: return u8"EtwRealtimeDisconnectCode"; // maybe in HANDLE
				case 15: return u8"EtwRegisterGuidsCode";
				case 16: return u8"EtwReceiveNotification";
				case 17: return u8"EtwSendDataBlock"; // maybe ETW_ENABLE_NOTIFICATION_PACKET // maybe ETW_SESSION_NOTIFICATION_PACKET
				case 18: return u8"EtwSendReplyDataBlock";
				case 19: return u8"EtwReceiveReplyDataBlock";
				case 20: return u8"EtwWdiSemUpdate";
				case 21: return u8"EtwEnumTraceGuidList"; // out GUID[]
				case 22: return u8"EtwGetTraceGuidInfo"; // in GUID, out ETW_TRACE_GUID_INFO
				case 23: return u8"EtwEnumerateTraceGuids"; // out TRACE_GUID_PROPERTIES[]
				case 24: return u8"EtwRegisterSecurityProv";
				case 25: return u8"EtwReferenceTimeCode"; // in ULONG LoggerId, out ETW_REF_CLOCK
				case 26: return u8"EtwTrackBinaryCode"; // in HANDLE
				case 27: return u8"EtwAddNotificationEvent"; // (zz: maybe in HANDLE)
				case 28: return u8"EtwUpdateDisallowList";
				case 29: return u8"EtwSetEnableAllKeywordsCode";
				case 30: return u8"EtwSetProviderTraitsCode";
				case 31: return u8"EtwUseDescriptorTypeCode";
				case 32: return u8"EtwEnumTraceGroupList";
				case 33: return u8"EtwGetTraceGroupInfo";
				case 34: return u8"EtwGetDisallowList";
				case 35: return u8"EtwSetCompressionSettings";
				case 36: return u8"EtwGetCompressionSettings";
				case 37: return u8"EtwUpdatePeriodicCaptureState";
				case 38: return u8"EtwGetPrivateSessionTraceHandle";
				case 39: return u8"EtwRegisterPrivateSession";
				case 40: return u8"EtwQuerySessionDemuxObject";
				case 41: return u8"EtwSetProviderBinaryTracking";
				case 42: return u8"EtwMaxLoggers"; // out ULONG
				case 43: return u8"EtwMaxPmcCounter"; // out ULONG
				case 44: return u8"EtwQueryUsedProcessorCount"; // ULONG // since WIN11
				case 45: return u8"EtwGetPmcOwnership";
				case 46: return u8"EtwGetPmcSessions";
				}
				utils_throw();
			}();
			return utils::text::format::format(
				u8"FunctionCode=%s,InputBuffer=\"unknown\",InputBufferLength=0x%x,OutputBuffer=\"unknown\",OutputBufferLength=0x%x,ReturnLength=unknown",
				FunctionCode.c_str(),
				m_reader.m_args[2],
				m_reader.m_args[4]);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtTraceControl", (NtTraceControl*)nullptr);});
	};
	class NtCreateFile : public NtdllNtXxxx
	{
	public:
		inner::nt_struct::OBJECT_ATTRIBUTES m_ObjectAttributes;
		inner::struct_data::NullablePointer<inner::nt_struct::LARGE_INTEGER> m_AllocationSize;
		inner::struct_data::NullablePointer<inner::struct_data::NATIVE_DATA> m_EaBuffer;

		struct s_return
		{
			uint64_t m_FileHandle{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 11);

			m_ObjectAttributes.read(hProcess, m_reader.m_args[2]);
			m_EaBuffer.read(hProcess, m_reader.m_args[9], (uint32_t)m_reader.m_args[10]);
		}
		std::u8string get_text_for_args() const override
		{
			return std::u8string()
				+ u8"ObjectAttributes=" + m_ObjectAttributes.to_text()
				+ u8",DesiredAccess=" + get_text_for_DesiredAccess(*(uint32_t*)&m_reader.m_args[1], *(uint32_t*)&m_reader.m_args[8])
				+ u8",IoStatusBlock=unknown"
				+ u8",AllocationSize=" + m_AllocationSize.to_text()
				+ u8",FileAttributes=" + get_text_for_FileAttributes(*(uint32_t*)&m_reader.m_args[5])
				+ u8",ShareAccess=" + get_text_for_ShareAccess(*(uint32_t*)&m_reader.m_args[6])
				+ u8",CreateDisposition=" + get_text_for_CreateDisposition(*(uint32_t*)&m_reader.m_args[7])
				+ u8",CreateOptions=" + get_text_for_CreateOptions(*(uint32_t*)&m_reader.m_args[8])
				+ u8",EaBuffer=" + m_EaBuffer.to_text()
				+ u8",EaLength=" + utils::windows::debugger::to_text::Size(*(uint32_t*)&m_reader.m_args[10]);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_return->m_FileHandle, sizeof(m_return->m_FileHandle));
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			return NtdllNtXxxx::get_text_for_return_value()
				+ u8",FileHandle=" + utils::windows::debugger::to_text::HANDLE(m_return->m_FileHandle);
		}
	public:
		static std::u8string get_text_for_DesiredAccess(uint32_t DesiredAccess, uint32_t CreateOptions)
		{
			auto type = (CreateOptions & 0x00000001/*FILE_DIRECTORY_FILE*/) ? u8"directory" : u8"file";

			return utils::windows::debugger::to_text::nt_type::ACCESS_MASK(DesiredAccess, type);
		}
		static std::u8string get_text_for_FileAttributes(uint32_t value)
		{
			utils::windows::debugger::to_text::flag::Bits<uint32_t> text{value};
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_ARCHIVE));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_ENCRYPTED));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_HIDDEN));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_NORMAL));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_OFFLINE));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_READONLY));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_SYSTEM));
			text.test(utils_macro__A_and_textA(FILE_ATTRIBUTE_TEMPORARY));
			return text.get_text();
		}
		static std::u8string get_text_for_ShareAccess(uint32_t value)
		{
			utils::windows::debugger::to_text::flag::Bits<uint32_t> text{value};
			text.test(utils_macro__A_and_textA(FILE_SHARE_READ));
			text.test(utils_macro__A_and_textA(FILE_SHARE_WRITE));
			text.test(utils_macro__A_and_textA(FILE_SHARE_DELETE));
			return text.get_text();
		}
		static std::u8string get_text_for_CreateDisposition(uint32_t value)
		{
			utils::windows::debugger::to_text::flag::Number<uint32_t> text{value};
			text.test(utils_macro__A_and_textA(FILE_SUPERSEDE));
			text.test(utils_macro__A_and_textA(FILE_OPEN));
			text.test(utils_macro__A_and_textA(FILE_CREATE));
			text.test(utils_macro__A_and_textA(FILE_OPEN_IF));
			text.test(utils_macro__A_and_textA(FILE_OVERWRITE));
			text.test(utils_macro__A_and_textA(FILE_OVERWRITE_IF));
			return text.get_text();
		}
		static std::u8string get_text_for_CreateOptions(uint32_t value)
		{
			utils::windows::debugger::to_text::flag::Bits<uint32_t> text{value};
			text.test(utils_macro__A_and_textA(FILE_DIRECTORY_FILE            ));
			text.test(utils_macro__A_and_textA(FILE_WRITE_THROUGH             ));
			text.test(utils_macro__A_and_textA(FILE_SEQUENTIAL_ONLY           ));
			text.test(utils_macro__A_and_textA(FILE_NO_INTERMEDIATE_BUFFERING ));
			text.test(utils_macro__A_and_textA(FILE_SYNCHRONOUS_IO_ALERT      ));
			text.test(utils_macro__A_and_textA(FILE_SYNCHRONOUS_IO_NONALERT   ));
			text.test(utils_macro__A_and_textA(FILE_NON_DIRECTORY_FILE        ));
			text.test(utils_macro__A_and_textA(FILE_CREATE_TREE_CONNECTION    ));
			text.test(utils_macro__A_and_textA(FILE_COMPLETE_IF_OPLOCKED      ));
			text.test(utils_macro__A_and_textA(FILE_NO_EA_KNOWLEDGE           ));
			text.test(utils_macro__A_and_textA(FILE_OPEN_REMOTE_INSTANCE      ));
			text.test(utils_macro__A_and_textA(FILE_RANDOM_ACCESS             ));
			text.test(utils_macro__A_and_textA(FILE_DELETE_ON_CLOSE           ));
			text.test(utils_macro__A_and_textA(FILE_OPEN_BY_FILE_ID           ));
			text.test(utils_macro__A_and_textA(FILE_OPEN_FOR_BACKUP_INTENT    ));
			text.test(utils_macro__A_and_textA(FILE_NO_COMPRESSION            ));
			text.test(utils_macro__A_and_textA(FILE_OPEN_REQUIRING_OPLOCK     ));
			text.test(0x00020000, u8"FILE_DISALLOW_EXCLUSIVE");
			text.test(0x00040000, u8"FILE_SESSION_AWARE");
			text.test(0x00100000, u8"FILE_RESERVE_OPFILTER");
			text.test(0x00200000, u8"FILE_OPEN_REPARSE_POINT");
			text.test(0x00400000, u8"FILE_OPEN_NO_RECALL");
			text.test(0x00800000, u8"FILE_OPEN_FOR_FREE_SPACE_QUERY");
			text.test(0x10000000, u8"FILE_CONTAINS_EXTENDED_CREATE_INFORMATION");
			return text.get_text();
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtCreateFile", (NtCreateFile*)nullptr);});
	};
	class NtOpenFile : public NtdllNtXxxx
	{
	public:
		inner::nt_struct::OBJECT_ATTRIBUTES m_ObjectAttributes;

		struct s_return
		{
			uint64_t m_FileHandle{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 6);

			m_ObjectAttributes.read(hProcess, m_reader.m_args[2]);
		}
		std::u8string get_text_for_args() const override
		{
			return std::u8string()
				+ u8"ObjectAttributes=" + m_ObjectAttributes.to_text()
				+ u8",DesiredAccess=" + NtCreateFile::get_text_for_DesiredAccess(*(uint32_t*)&m_reader.m_args[1], *(uint32_t*)&m_reader.m_args[5])
				+ u8",IoStatusBlock=unknown"
				+ u8",ShareAccess=" + NtCreateFile::get_text_for_ShareAccess(*(uint32_t*)&m_reader.m_args[4])
				+ u8",OpenOptions=" + NtCreateFile::get_text_for_CreateOptions(*(uint32_t*)&m_reader.m_args[5]);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_return->m_FileHandle, sizeof(m_return->m_FileHandle));
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			return NtdllNtXxxx::get_text_for_return_value() + utils::text::format::format(
				u8",FileHandle=0x%llx",
				m_return->m_FileHandle);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtOpenFile", (NtOpenFile*)nullptr);});
	};
	class NtCreateWorkerFactory : public NtdllNtXxxx
	{
	public:
		inner::struct_data::NullablePointer<inner::nt_struct::OBJECT_ATTRIBUTES> m_ObjectAttributes;

		struct s_return
		{
			uint64_t m_WorkerFactoryHandle{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 10);

			m_ObjectAttributes.read(hProcess, m_reader.m_args[2]);
		}
		std::u8string get_text_for_args() const override
		{
			namespace to_text = utils::windows::debugger::to_text;
			return std::u8string{}
				+ u8"DesiredAccess=" + to_text::nt_type::ACCESS_MASK((uint32_t)m_reader.m_args[1], u8"unknown")
				+ u8",ObjectAttributes=" + m_ObjectAttributes.to_text()
				+ u8",CompletionPortHandle=" + to_text::HANDLE(m_reader.m_args[3])
				+ u8",WorkerProcessHandle=" + to_text::HANDLE(m_reader.m_args[4])
				+ u8",StartRoutine=" + to_text::Addr(m_reader.m_args[5])
				+ u8",StartParameter=" + to_text::Addr(m_reader.m_args[6])
				+ u8",MaxThreadCount=" + to_text::Size((uint32_t)m_reader.m_args[7])
				+ u8",StackReserve=" + to_text::Size((uint64_t)m_reader.m_args[8])
				+ u8",StackCommit=" + to_text::Size((uint64_t)m_reader.m_args[9]);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_return->m_WorkerFactoryHandle, sizeof(m_return->m_WorkerFactoryHandle));
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			namespace to_text = utils::windows::debugger::to_text;
			return NtdllNtXxxx::get_text_for_return_value()
				+ u8",WorkerFactoryHandle=" + to_text::HANDLE(m_return->m_WorkerFactoryHandle);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtCreateWorkerFactory", (NtCreateWorkerFactory*)nullptr);});
	};
	class NtCreateIoCompletion : public NtdllNtXxxx
	{
	public:
		inner::struct_data::NullablePointer<inner::nt_struct::OBJECT_ATTRIBUTES> m_ObjectAttributes;

		struct s_return
		{
			uint64_t m_IoCompletionHandle{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 4);

			m_ObjectAttributes.read(hProcess, m_reader.m_args[2]);
		}
		std::u8string get_text_for_args() const override
		{
			namespace to_text = utils::windows::debugger::to_text;
			return std::u8string{}
				+ u8"DesiredAccess=" + to_text::nt_type::ACCESS_MASK((uint32_t)m_reader.m_args[1], u8"io_completion_port")
				+ u8",ObjectAttributes=" + m_ObjectAttributes.to_text()
				+ u8",NumberOfConcurrentThreads=" + to_text::Size((uint32_t)m_reader.m_args[3]);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_return->m_IoCompletionHandle, sizeof(m_return->m_IoCompletionHandle));
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			namespace to_text = utils::windows::debugger::to_text;
			return NtdllNtXxxx::get_text_for_return_value()
				+ u8",IoCompletionHandle=" + to_text::HANDLE(m_return->m_IoCompletionHandle);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtCreateIoCompletion", (NtCreateIoCompletion*)nullptr);});
	};
	class NtQueryWnfStateData : public NtdllNtXxxx
	{
	public:
		uint64_t m_StateName{};
		inner::struct_data::NullablePointer<inner::struct_data::GUID> m_TypeId;
		uint32_t m_BufferSize{};

		struct s_return
		{
			uint32_t m_ChangeStamp{};
			inner::struct_data::NullablePointer<inner::struct_data::NATIVE_DATA> m_Buffer;
			uint32_t m_BufferSize{};
		};
		std::optional<s_return> m_return;
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 6);

			utils::windows::memory::read_memory(hProcess, m_reader.m_args[0], &m_StateName, sizeof(m_StateName));
			m_TypeId.read(hProcess, m_reader.m_args[1]);
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[5], &m_BufferSize, sizeof(m_BufferSize));
		}
		std::u8string get_text_for_args() const override
		{
			namespace to_text = utils::windows::debugger::to_text;
			return std::u8string{}
				+ u8"StateName=" + to_text::Size(m_StateName)
				+ u8",TypeId=" + m_TypeId.to_text()
				+ u8",BufferSize=" + to_text::Size(m_BufferSize);
		}
		void read_return_value(HANDLE hProcess, HANDLE hThread) override
		{
			NtdllNtXxxx::read_return_value(hProcess, hThread);

			m_return = s_return{};
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[3], &m_return->m_ChangeStamp, sizeof(m_return->m_ChangeStamp));
			utils::windows::memory::read_memory(hProcess, m_reader.m_args[5], &m_return->m_BufferSize, sizeof(m_return->m_BufferSize));
			m_return->m_Buffer.read(hProcess, m_reader.m_args[4], m_return->m_BufferSize);
		}
		std::u8string get_text_for_return_value() override
		{
			//简单检查
			utils_assert(m_return.has_value());

			namespace to_text = utils::windows::debugger::to_text;
			return NtdllNtXxxx::get_text_for_return_value()
				+ u8",ChangeStamp=" + to_text::TimeStamp(m_return->m_ChangeStamp)
				+ u8",Buffer=" + m_return->m_Buffer.to_text()
				+ u8",BufferSize=" + to_text::Size(m_return->m_BufferSize);
		}
		std::u8string get_text_for_note() const override
		{
			const auto & state = utils::windows::advanced::wnf::query(m_StateName);
			return std::u8string{}
				+ u8"MacroName=" + state.MacroName
				+ u8",Note=\"" + state.Note + u8"\"";
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtQueryWnfStateData", (NtQueryWnfStateData*)nullptr);});
	};
	class NtTerminateProcess : public NtdllNtXxxx
	{
#if 0
		NTSTATUS NtTerminateProcess(
    HANDLE ProcessHandle,
    NTSTATUS ExitStatus
);
#endif
		
	public:
		void read_args(HANDLE hProcess, HANDLE hThread) override
		{
			m_reader.read_stack_args(hProcess, hThread, 2);
		}
		std::u8string get_text_for_args() const override
		{
			namespace to_text = utils::windows::debugger::to_text;
			return std::u8string{}
				+ u8"ProcessHandle=" + to_text::HANDLE(m_reader.m_args[0])
				+ u8",ExitStatus=" + to_text::NTSTATUS((uint32_t)m_reader.m_args[1]);
		}
	private:
		CallWhenProgramStart([](){inner::register_syscall(u8"NtTerminateProcess", (NtTerminateProcess*)nullptr);});
	};
}