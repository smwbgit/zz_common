#pragma once

#include <windows.h>
#include <utils/throw.hpp>

//一个基本的调试器
namespace utils::windows::debug::debugger::basic
{
	//处理最基本的事项
	class DebuggerCore
	{
		//请指定
	public:
		std::u8string m_cmdline;
		utils::functions<void()> m_before_loop;
		utils::functions<void()> m_before_exit;

		//请指定: 各个调试事件的处理
		//note: 向其中添加自己的处理逻辑
	public:
		utils::functions<void()> m_breakpoint_event;
		utils::functions<void()> m_single_step_event;
		utils::functions<void()> m_create_process_event;
		utils::functions<void()> m_exit_process_event;
		utils::functions<void()> m_create_thread_event;
		utils::functions<void()> m_exit_thread_event;
		utils::functions<void()> m_load_dll_event;
		utils::functions<void()> m_unload_dll_event;

	public:
		std::map<std::u8string, uint64_t> m_统计;
		const PROCESS_INFORMATION m_pi{};
		const DEBUG_EVENT m_debugEvent{};	//每次的调试事件
		DWORD m_continueStatus{};			//每次的继续状态

	public:
		//设置好各种参数后，调用此函数。直到被调试进程退出，此函数才返回
		void run()
		{
			STARTUPINFO si{}; si.cb = sizeof(si);
			std::u16string cmdline_u16 = utils::text::encoding::unicode::utf8_to_utf16(m_cmdline);
			utils_assert(CreateProcessW(
				NULL, (wchar_t*)cmdline_u16.c_str(),
				NULL,                // 进程安全属性
				NULL,                // 线程安全属性
				FALSE,               // 是否继承句柄
				DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,       // 创建调试进程
				NULL,                // 使用父进程的环境块
				NULL,                // 使用父进程的起始目录
				&si,
				const_cast<PROCESS_INFORMATION*>(&m_pi)));

			m_before_loop.call();

			bool is_finish{};
			while (!is_finish)
			{
				utils_assert(WaitForDebugEvent(const_cast<DEBUG_EVENT*>(&m_debugEvent), INFINITE));

				m_continueStatus = DBG_EXCEPTION_NOT_HANDLED;
				switch (m_debugEvent.dwDebugEventCode)
				{
				case EXCEPTION_DEBUG_EVENT:
					switch (m_debugEvent.u.Exception.ExceptionRecord.ExceptionCode)
					{
					case EXCEPTION_ACCESS_VIOLATION:
						// First chance: Pass this on to the system. 
						// Last chance: Display an appropriate error. 
						break;

					case EXCEPTION_BREAKPOINT:
						on_breakpoint_event();
						break;

					case EXCEPTION_DATATYPE_MISALIGNMENT:
						// First chance: Pass this on to the system. 
						// Last chance: Display an appropriate error. 
						break;

					case EXCEPTION_SINGLE_STEP:
						on_single_step_event();
						break;

					case DBG_CONTROL_C:
						// First chance: Pass this on to the system. 
						// Last chance: Display an appropriate error. 
						break;

					default:
						// Handle other exceptions. 
						utils_throw();
						break;
					}
					break;

				case CREATE_THREAD_DEBUG_EVENT:
					on_create_thread_event();
					break;

				case CREATE_PROCESS_DEBUG_EVENT:
					on_create_process_event();
					break;

				case EXIT_THREAD_DEBUG_EVENT:
					on_exit_thread_event();
					break;

				case EXIT_PROCESS_DEBUG_EVENT:
					is_finish = true;	// 被调试进程已退出，退出调试循环

					on_exit_process_event();
					break;

				case LOAD_DLL_DEBUG_EVENT:
					on_load_dll_event();
					break;

				case UNLOAD_DLL_DEBUG_EVENT:
					on_unload_dll_event();
					break;

				case OUTPUT_DEBUG_STRING_EVENT:
					// Display the output debugging string. 
					break;

				case RIP_EVENT:
					break;

				default:
					utils_throw();
				}

				// 继续被调试的进程
				utils_assert(ContinueDebugEvent(m_debugEvent.dwProcessId, m_debugEvent.dwThreadId,
					m_continueStatus));
			}
			
			m_before_exit.call();
		}
		//读取被调试进程的内存
		void read_debuggee_memory(void* buffer, uint64_t size, uint64_t addr)
		{
			utils::windows::memory::read_protected_memory(m_pi.hProcess, addr, buffer, size);
		}
		//特化
		template<typename T, typename = std::enable_if_t< utils::is_arithmetic_v<T> >>
		void read_debuggee_memory(T & value, uint64_t addr)
		{
			read_debuggee_memory(&value, sizeof(value), addr);
		}
		//读取被调试进程的内存 (被修改前的内存)
		void read_debuggee_memory_original(void* buffer, uint64_t size, uint64_t addr)
		{
			//todo 提供正确数据
			read_debuggee_memory(buffer, size, addr);
		}
		//写入被调试进程的内存
		void write_debuggee_memory(const void* buffer, uint64_t size, uint64_t addr)
		{
			utils::windows::memory::write_protected_memory(m_pi.hProcess, addr, {buffer, size});
		}
		//特化
		template<typename T, typename = std::enable_if_t< utils::is_arithmetic_v<T> >>
		void write_debuggee_memory(const T & value, uint64_t addr)
		{
			return write_debuggee_memory(&value, sizeof(value), addr);
		}
		//特化
		void write_debuggee_memory(const utils::data & data, uint64_t addr)
		{
			return write_debuggee_memory(data.addr(), data.size(), addr);
		}
	private:
		void on_breakpoint_event()
		{
			m_统计[u8"breakpoint"]++;
			
			//转发
			m_breakpoint_event.call();

			if(m_continueStatus == DBG_EXCEPTION_NOT_HANDLED)
				m_统计[u8"breakpoint.not_handled"]++;
		}
		void on_single_step_event()
		{
			m_统计[u8"single_step"]++;

			//转发
			m_single_step_event.call();
		}
		void on_create_process_event()
		{
			m_统计[u8"create_process"]++;

			//简单检查
			utils_assert(m_debugEvent.u.CreateProcessInfo.hFile);
			utils_assert(m_debugEvent.u.CreateProcessInfo.hProcess);
			utils_assert(m_debugEvent.u.CreateProcessInfo.hThread);

			//转发
			m_create_process_event.call();

			utils_assert(CloseHandle(m_debugEvent.u.CreateProcessInfo.hFile));

			/*不需要关闭*/   //utils_assert(CloseHandle(m_debugEvent.u.CreateProcessInfo.hProcess));
			/*不需要关闭*/   //utils_assert(CloseHandle(m_debugEvent.u.CreateProcessInfo.hThread));
		}
		void on_exit_process_event()
		{
			m_统计[u8"exit_process"]++;
			
			//转发
			m_exit_process_event.call_reverse();
		}
		void on_create_thread_event()
		{
			m_统计[u8"create_thread"]++;

			const auto hThread = m_debugEvent.u.CreateThread.hThread;

			//简单检查
			utils_assert(hThread);
			
			//转发
			m_create_thread_event.call();

			/*不需要关闭*/   //utils_assert(CloseHandle(hThread));
		}
		void on_exit_thread_event()
		{
			m_统计[u8"exit_thread"]++;
			
			//转发
			m_exit_thread_event.call_reverse();

			/*不需要关闭*/   //utils_assert(CloseHandle(hThread));
		}
		void on_load_dll_event()
		{
			m_统计[u8"load_dll"]++;
			
			//转发
			m_load_dll_event.call();

			//关闭句柄
			utils_assert(CloseHandle(m_debugEvent.u.LoadDll.hFile));
		}
		void on_unload_dll_event()
		{
			m_统计[u8"unload_dll"]++;
			
			//转发
			m_unload_dll_event.call_reverse();
		}
	};
	
	//功能模块: 管理模块的加载和卸载
	class DebuggerModule_Modules : public DebuggerCore
	{
	public:
		utils::windows::process::ModuleInfoManager m_module_info_manager;

	public:
		DebuggerModule_Modules()
		{
			//打开选项：读取dll文件
			m_module_info_manager.m_is_load_file = true;
			
			//添加exe模块
			m_create_process_event += [this]()
			{
				auto & u = m_debugEvent.u.CreateProcessInfo;
				m_module_info_manager.add(u.hFile, (uint64_t)u.lpBaseOfImage);
			};
			//添加后续的dll模块
			m_load_dll_event += [this]()
			{
				auto & u = m_debugEvent.u.LoadDll;
				m_module_info_manager.add(u.hFile, (uint64_t)u.lpBaseOfDll);
			};
			//移除卸载的dll模块
			m_unload_dll_event += [this]()
			{
				auto & u = m_debugEvent.u.UnloadDll;
				m_module_info_manager.remove((uint64_t)u.lpBaseOfDll);
			};
		}
	};

	//功能模块: 管理线程的创建和结束
	class DebuggerModule_Threads : public DebuggerModule_Modules
	{
	public:
		struct s_thread
		{
			DWORD m_tid{};
			HANDLE m_hThread{};
		};
		std::map<DWORD,s_thread> m_all_thread;

	public:
		DebuggerModule_Threads()
		{
			//添加事件处理
			m_create_process_event += [this]()
			{
				auto & u = m_debugEvent.u.CreateProcessInfo;

				s_thread t{};
				t.m_tid = m_debugEvent.dwThreadId;
				t.m_hThread = u.hThread;
				utils::map(m_all_thread).insert_not_exist(t.m_tid, t);
			};
			//添加事件处理
			m_create_thread_event += [this]()
			{
				auto & u = m_debugEvent.u.CreateThread;

				s_thread t{};
				t.m_tid = m_debugEvent.dwThreadId;
				t.m_hThread = u.hThread;
				utils::map(m_all_thread).insert_not_exist(t.m_tid, t);
			};
			//添加事件处理
			m_exit_thread_event += [this]()
			{
				utils::map(m_all_thread).remove(m_debugEvent.dwThreadId);
			};
		}
		s_thread & get_thread(DWORD tid)
		{
			return utils::map(m_all_thread).get_existed_value(tid);
		}
		//获得当前线程
		s_thread & get_current_thread()
		{
			return get_thread(m_debugEvent.dwThreadId);
		}
	};

	//功能模块: 管理断点的添加和删除
	class DebuggerModule_Breakpoints : public DebuggerModule_Threads
	{
	public:
		//继续方式
		enum class ContinueMethod
		{
			normal = 1,
			no_return,	//恢复0xcc, 但不等待单步结果
			have_been_moved,	//rip已被移走，不用恢复0xcc
		};
	private:
		struct s_user
		{
			uint64_t m_id{};
			std::function<void()> m_meet_one_breakpoint;
		};
		struct s_breakpoint
		{
			uint64_t m_addr{};
			uint8_t m_old_byte{};
			ContinueMethod m_continue_method{};
			bool m_is_arrive_at{};	//当前是否断在这个断点上。如果为真，则删除断点时，保留这个结构，断点继续后，再删除。如果为假，则直接删除。
			std::map<uint64_t, s_user> m_users;
		};

		//此功能模块用到的数据
		struct s_this_module
		{
			std::map<uint64_t, s_breakpoint> m_all_breakpoint;
			std::map<uint64_t /* id */, uint64_t /* breakpoint.m_addr */> m_all_user;
			utils::IdGiver<uint64_t> m_id_giver;
		};
		s_this_module  m_this_module{};

	public:
		DebuggerModule_Breakpoints()
		{
			//添加对断点事件的处理
			m_breakpoint_event += [this]()
			{
				//断点位置
				auto & u = m_debugEvent.u.Exception;
				const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;

				//此断点没有记录
				if (!utils::map(m_this_module.m_all_breakpoint).is_exist(addr))
					return;

				//简单检查: 没有别人处理过
				utils_assert(m_continueStatus == DBG_EXCEPTION_NOT_HANDLED);

				//简单检查
				utils_assert(u.dwFirstChance);

				//查找断点信息
				auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);
				
				//简单检查
				utils_assert(!breakpoint.m_is_arrive_at);

				//锁定断点信息
				breakpoint.m_is_arrive_at = true;

				//获得所有客户id。防止在通知过程中，有向此断点位置添加或删除断点的情况时，影响遍历结果
				const auto ids = utils::map(breakpoint.m_users).get_all_key();

				//通知客户
				for(const auto id : ids)
				{
					//如果客户id已经不在，说明在遍历过程中，已经被删除了，直接跳过
					if (!utils::map(breakpoint.m_users).is_exist(id)) continue;

					//获得客户信息
					const auto & user = utils::map(breakpoint.m_users).get_existed_value(id);

					//通知
					user.m_meet_one_breakpoint();
				}
				
				//简单检查
				utils_assert(breakpoint.m_is_arrive_at);

				//释放锁定
				breakpoint.m_is_arrive_at = false;

				//如果已没有客户，移除断点并返回
				if (breakpoint.m_users.empty())
					return continue_with_no_user();

				//继续断点：根据设置，做不同的处理
				switch(breakpoint.m_continue_method)
				{
				//普通断点, 正常处理
				case ContinueMethod::normal:
					return continue_with_normal();

				//恢复0xcc, 但不等待单步结果
				case ContinueMethod::no_return:
					return continue_with_no_return();

				//rip已被移走，不用恢复0xcc
				case ContinueMethod::have_been_moved:
					return continue_with_have_been_moved();
				}
				utils_throw();
			};
		}
		//添加断点
		//addr: 断点地址
		//meet_one_breakpoint: 当断点断下时的回调函数
		//返回值: 标识断点, 当删除断点时使用
		//note: 如果断点位置已经被设置过断点，并不会报错，而且callback依旧会被通知到
		uint64_t add_breakpoint(const uint64_t addr, ContinueMethod continue_method, std::function<void()> meet_one_breakpoint)
		{
			//生成用户id
			const auto id = m_this_module.m_id_giver.get_one_id();

			//此位置已经被下过断点
			if(utils::map(m_this_module.m_all_breakpoint).is_exist(addr))
			{
				//获得断点信息
				auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);
				
				//检查继续类型是否相同
				utils_assert(breakpoint.m_continue_method == continue_method);

				//增加用户
				utils::map(breakpoint.m_users).insert_not_exist(id, {id, meet_one_breakpoint});
				
				//同步更新索引
				utils::map(m_this_module.m_all_user).insert_not_exist(id, addr);

				//给出用户id
				return id;
			}

			//新建断点信息
			{
				//填充数据
				s_breakpoint breakpoint{};
				breakpoint.m_addr = addr;
				breakpoint.m_continue_method = continue_method;
				//breakpoint.m_is_arrive_at = false;
				//breakpoint.m_old_byte = 0;
				breakpoint.m_users[id] = {id, meet_one_breakpoint};

				//保存
				utils::map(m_this_module.m_all_breakpoint).insert_not_exist(addr, std::move(breakpoint));
				
				//同步更新索引
				utils::map(m_this_module.m_all_user).insert_not_exist(id, addr);
			}
			
			//获取断点信息
			auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);
				
			//设置断点字节
			set_breakpoint_byte(breakpoint);

			//给出用户id
			return id;
		}
		//特化
		std::vector<uint64_t> add_breakpoints(const std::vector<uint64_t> & addrs,
			ContinueMethod continue_method, std::function<void()> meet_one_breakpoint)
		{
			std::vector<uint64_t> ids;
			for(const auto addr : addrs)
			{
				const auto id = add_breakpoint(addr, ContinueMethod::have_been_moved,
					meet_one_breakpoint);

				ids.push_back(id);
			}
			return ids;
		}
		//移除断点
		//id: 设置断点时返回的标识
		//note: 如果断点位置没有被设置过断点，则会报错
		void remove_breakpoint(uint64_t id)
		{
			//获得断点地址
			const auto addr = utils::map(m_this_module.m_all_user).get_existed_value(id);
			
			//获得断点信息
			auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);

			//移除用户
			utils::map(breakpoint.m_users).remove(id);
			
			//同步更新索引
			utils::map(m_this_module.m_all_user).remove(id);
			
			//如果没有用户，并且断点没有被锁定，则真正删除断点
			if (breakpoint.m_users.empty() && !breakpoint.m_is_arrive_at)
			{
				//恢复原始字节
				restore_breakpoint_byte(breakpoint);

				//删除断点信息
				remove_breakpoint_information(breakpoint);
			}
		}
		//特化
		void remove_breakpoints(const std::vector<uint64_t> & ids)
		{
			for(const auto id : ids)
				remove_breakpoint(id);
		}

	private:
		//删除断点信息
		void remove_breakpoint_information(s_breakpoint & breakpoint)
		{
			//简单检查
			utils_assert(breakpoint.m_users.empty());
			utils_assert(!breakpoint.m_is_arrive_at);

			//删除断点信息
			utils::map(m_this_module.m_all_breakpoint).remove(breakpoint.m_addr);
		}
		//设置断点字节
		void set_breakpoint_byte(s_breakpoint & breakpoint)
		{
			//断点地址
			const auto addr = breakpoint.m_addr;

			//读取原始字节
			uint8_t c{};
			read_debuggee_memory(c, addr);

			//保存
			breakpoint.m_old_byte = c;
				
			//设置新的字节
			c = 0xcc;
			write_debuggee_memory(c, addr);

			//刷新缓存
			flush_instruction_cache(addr);
		}
		//恢复原始字节
		void restore_breakpoint_byte(s_breakpoint & breakpoint)
		{
			//断点地址
			const auto addr = breakpoint.m_addr;

			//读取原始字节
			uint8_t c{};
			read_debuggee_memory(c, addr);

			//简单检查
			utils_assert(c == 0xcc);
				
			//设置新的字节
			c = breakpoint.m_old_byte;
			write_debuggee_memory(c, addr);

			//刷新缓存
			flush_instruction_cache(addr);
		}
		//刷新指令缓存
		void flush_instruction_cache(uint64_t addr)
		{
			//刷新缓存
			utils_assert(FlushInstructionCache(m_pi.hProcess, (LPCVOID)addr, 1));
		}
		//继续一个没有客户的断点
		void continue_with_no_user()
		{
			//断点位置
			auto & u = m_debugEvent.u.Exception;
			const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;

			//获得断点信息
			auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);
			
			//恢复原始字节
			restore_breakpoint_byte(breakpoint);

			//往回偏移一下rip, 重新执行0xcc处的指令
			auto & thread = get_thread(m_debugEvent.dwThreadId);
			utils::windows::debugger::shift_rip(thread.m_hThread, -1);

			//删除断点信息
			remove_breakpoint_information(breakpoint);

			//更新状态
			m_continueStatus = DBG_CONTINUE;
		}
		//继续一个普通断点
		void continue_with_normal()
		{
			//断点位置
			auto & u = m_debugEvent.u.Exception;
			const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;

			//获得断点信息
			auto & breakpoint = utils::map(m_this_module.m_all_breakpoint).get_existed_value(addr);

			//恢复原始字节
			restore_breakpoint_byte(breakpoint);

			//暂停其他线程
			for(auto & one : m_all_thread)
			{
				if (one.second.m_tid == m_debugEvent.dwThreadId) continue;
				utils::windows::process::suspend_thread(one.second.m_hThread);
			}

			//设置为单步运行
			auto & thread = get_thread(m_debugEvent.dwThreadId);
			utils::windows::debugger::set_single_step(thread.m_hThread);
				
			//往回偏移一下rip, 重新执行0xcc处的指令
			utils::windows::debugger::shift_rip(thread.m_hThread, -1);

			//运行事件循环，等待单步事件
			utils_assert(ContinueDebugEvent(m_debugEvent.dwProcessId, m_debugEvent.dwThreadId,
				DBG_CONTINUE));
			while(1)
			{
				DEBUG_EVENT debugEvent{};
				utils_assert(WaitForDebugEvent(&debugEvent, INFINITE));

				//简单检查
				utils_assert(debugEvent.dwProcessId == m_debugEvent.dwProcessId);

				//不是此线程，放回去
				if(debugEvent.dwThreadId != m_debugEvent.dwThreadId)
				{
					utils_assert(ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId,
						DBG_REPLY_LATER));
					continue;
				}

				//简单检查: 单步事件
				utils_assert(debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT);
				utils_assert(debugEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP);
				const auto _addr = (uint64_t)debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;
				utils_assert(_addr > addr && _addr - addr <= 15);

				break;
			}

			//重新打上断点
			set_breakpoint_byte(breakpoint);
				
			//恢复其他线程
			for(auto & one : m_all_thread)
			{
				if (one.second.m_tid == m_debugEvent.dwThreadId) continue;
				utils::windows::process::resume_thread(one.second.m_hThread);
			}

			//更新状态
			m_continueStatus = DBG_CONTINUE;
		}
		//继续一个返回到系统的断点
		void continue_with_no_return()
		{
			//todo: 待实现
			utils_throw();
		}
		//继续一个挪移断点
		void continue_with_have_been_moved()
		{
			//不恢复原始字节

			//更新状态
			m_continueStatus = DBG_CONTINUE;
		}
	};
	
	//一个基本的调试器
	//note: 由于指针转换的原因，提前声明。把最后一个功能模块放在下面了
	class DebuggerModule_OptionalFeatures;
	using BasicDebugger = DebuggerModule_OptionalFeatures;

	//功能模块: 管理可选功能
	class DebuggerModule_OptionalFeatures : public DebuggerModule_Breakpoints
	{
	public:
		//可选功能的基类
		class OptionalFeature
		{
		protected:
			friend class DebuggerModule_OptionalFeatures;
			BasicDebugger* m_basic_debugger{};
		};
	private:
		//此功能模块用到的数据
		struct s_this_module
		{
			std::map<std::u8string, std::unique_ptr<OptionalFeature>> m_all_optional_feature;
		};
		s_this_module  m_this_module{};

	public:
		//添加可选功能
		template<typename T>
		void add_optional_feature()
		{
			auto feature = std::make_unique<T>();
			feature->m_basic_debugger = this;
			utils::map(m_this_module.m_all_optional_feature).insert_not_exist(get_name<T>(),
				std::move(feature));
		}
		//获得可选功能的对象
		template<typename T>
		T & get_optional_feature()
		{
			auto feature = utils::map(m_this_module.m_all_optional_feature).get_existed_value(get_name<T>()).get();
			return *static_cast<T*>(feature);
		}
	private:
		template<typename T>
		std::u8string get_name()
		{
			return (const char8_t*)typeid(T).name();
		}
	};
}

namespace utils::windows::debug::debugger
{
	using BasicDebugger = utils::windows::debug::debugger::basic::BasicDebugger;
}