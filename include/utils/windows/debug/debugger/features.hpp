#pragma once

#include <deque>
#include <utils/windows/debug/debugger/basic.hpp>
#include <utils/windows/unwind.hpp>
#include <utils/deque.hpp>

namespace utils::windows::debug
{
	//函数签名
	struct FunctionSignature
	{
		struct s_pointer
		{
			uint64_t offset{};
			std::u8string type;
		};
		struct s_struct
		{
			uint64_t size{};
			std::vector<s_pointer> pointers;
		};

		std::u8string m_return_value;
		std::vector<std::u8string> m_params;
	};
}
//可选功能
namespace utils::windows::debug::debugger::features
{
	namespace inner
	{
		using OptionalFeature = utils::windows::debug::debugger::basic::DebuggerModule_OptionalFeatures::OptionalFeature;
	}

#if 0
	//在ntdll.dll的所有syscall处下断点
	//note: 简单使用断点，但存在问题是，有些syscall不会再返回了。
	class BreakAllSyscall_Simple : public inner::OptionalFeature
	{
	private:
		utils::Switch m_switch;
		bool m_is_first{true};	//是否未初始化
		utils::windows::process::ModuleInfoManager::s_module* m_module{};	//ntdll.dll
		std::map<uint64_t/*syscall的全局地址*/, std::u8string> m_all_syscall;
		uint64_t m_breakpoints_id{};

	public:
		//在ntdll.dll的所有syscall处下断点
		void turn_on(std::function<void()> meet_one_syscall)
		{
			auto _turn_on = [this, meet_one_syscall]()
			{
				//初始化
				if (m_is_first)
				{
					m_is_first = false;

					m_module = &m_basic_debugger->m_module_info_manager.
						get_module_by_full_name(u8R"(C:\Windows\System32\ntdll.dll)");

					//检查: ntdll.dll不该被卸载
					m_basic_debugger->m_unload_dll_event += [this]()
					{
						auto & u = m_basic_debugger->m_debugEvent.u.UnloadDll;

						//简单检查
						utils_assert((uint64_t)u.lpBaseOfDll != m_module->m_base);
					};

					m_all_syscall = [&]()
					{
						auto export_table = m_module->m_pe_viewer->get_export_table_detail();
						std::map<uint64_t/*syscall的地址*/, std::vector<std::u8string>> export_map;
						for(const auto & one : export_table)
						{
							//简单检查
							utils_assert(!one.is_forward);

							export_map[one.rva + m_module->m_base]
								.push_back((const std::u8string &)one.name);
						}
						std::map<uint64_t, std::u8string> all_syscall;
						for(const auto & one : export_map)
						{
							//检查指定位置是否有syscall指令
							auto export_func = one.first;
							uint16_t value{};
							m_basic_debugger->read_debuggee_memory(value, export_func + 0x12);
							if (0x050f != value)
								continue;

							//简单检查
							utils_assert(one.second.size() >= 2);
							utils_assert(one.second[0][0] == u8'N');
							utils_assert(one.second[0][1] == u8't');
							utils_assert(one.second[one.second.size()-1][0] == u8'Z');
							utils_assert(one.second[one.second.size()-1][1] == u8'w');
							utils_assert(one.second[0].substr(2) == one.second[one.second.size()-1].substr(2));

							all_syscall[export_func + 0x12] = one.second[0];
						}
						return all_syscall;
					}();
				}
				
				//简单检查
				utils_assert(!m_breakpoints_id);

				//设置断点
				std::vector<uint64_t> addrs = utils::map(m_all_syscall).get_all_key();
				m_breakpoints_id = m_basic_debugger->set_breakpoints(addrs, [meet_one_syscall]()
				{
					Sleep(1);
					meet_one_syscall();
				});
			};
			m_switch.turn_on(true, _turn_on);
		}
		//取消所有syscall处的断点
		void turn_off()
		{
			auto _turn_off = [this]()
			{
				//取消断点
				m_basic_debugger->remove_breakpoints(m_breakpoints_id);

				//更新状态
				m_breakpoints_id = 0;
			};
			m_switch.turn_off(true, _turn_off);
		}
	};
#endif

	//在ntdll.dll的所有syscall处下断点
	//note: 在被调试进程中开辟内存空间，让每个线程的syscall在自己单独的代码上运行，避免有些syscall不再返回的问题。
	//note: 需要ntdll.dll已经载入
	class BreakAllSyscall : public inner::OptionalFeature
	{
	private:
		struct s_syscall
		{
			uint64_t m_addr{};	//syscall的全局地址
			std::u8string m_name;
		};
		struct s_group
		{
			uint64_t m_addr{};
			std::optional<uint64_t> m_tid;
			const s_syscall* m_syscall{};
			bool m_is_wait_return{};	//是否在等待syscall返回
		};
		enum class StopWaitReason
		{
			another_syscall,
			exit_thread,
			shut_down,
			EnumSize,
		};

	private:
		utils::Switch m_switch;
		bool m_is_local_ready{};
		bool m_is_remote_ready{};
		utils::windows::process::ModuleInfoManager::s_module* m_module{};	//ntdll.dll
		std::map<uint64_t/*syscall的全局地址*/, s_syscall> m_all_syscall;
		std::vector<uint64_t> m_all_enter_breakpoint{};
		std::vector<uint64_t> m_all_exit_breakpoint{};
		uint64_t m_buffer_addr{};	//被调试进程中开辟的内存空间
		const uint64_t m_group_n{1000};	//支持的最大线程数（并行数量，历史线程不算）
		std::vector<s_group> m_all_group;
		std::map<uint64_t, s_group*> m_map_tid_to_group;
		std::vector<std::set<std::u8string>> m_all_syscall_no_return;
		std::function<void(uint64_t addr)> m_meet_syscall_return;
		std::function<void(uint64_t addr)> m_meet_syscall_no_return;

	public:
		//在ntdll.dll的所有syscall处下断点
		void turn_on(std::function<void()> meet_syscall_call,
			std::function<void(uint64_t addr /*syscall的地址*/)> meet_syscall_return,
			std::function<void(uint64_t addr /*syscall的地址*/)> meet_syscall_no_return)
		{
			//确保开关状态正确
			m_switch.turn_on();

			//确认远程和本地系统，都已准备完毕
			if (!m_is_local_ready) start_up_local();
			if (!m_is_remote_ready) start_up_remote();

			//获得syscall的地址
			std::vector<uint64_t> addrs = utils::map(m_all_syscall).get_all_key();

			//类型声明
			using ContinueMethod = utils::windows::debug::debugger::basic::
				DebuggerModule_Breakpoints::ContinueMethod;

			//简单检查
			utils_assert(m_all_enter_breakpoint.empty());

			//向每个syscall添加进入断点
			m_all_enter_breakpoint = m_basic_debugger->add_breakpoints(addrs,
				ContinueMethod::have_been_moved, [this, meet_syscall_call]()
				{
					//捕捉syscall，并让当前线程执行新开辟空间中放置的syscall指令
					on_enter_breakpoint(meet_syscall_call);
				});

			//保存返回回调
			m_meet_syscall_return = meet_syscall_return;
			m_meet_syscall_no_return = meet_syscall_no_return;
		}
		//取消所有syscall处的断点
		void turn_off()
		{
			//确保开关状态正确
			m_switch.turn_off();
			
			//简单检查
			utils_assert(!m_all_enter_breakpoint.empty());

			//删除进入断点
			m_basic_debugger->remove_breakpoints(m_all_enter_breakpoint);
			m_all_enter_breakpoint.clear();

			//取消返回回调
			m_meet_syscall_return = nullptr;
		}
		//关闭远程系统
		void shut_down_remote()
		{
			//确保没在使用此功能
			utils_assert(!m_switch.is_on());

			//简单检查
			utils_assert(m_is_remote_ready);

			//准备数据: 所有使用过syscall的线程
			//note: 之所以提前准备，是因为，归还内存块时，循环内会不断删除m_map_tid_to_group的元素，不好对其进行循环
			const auto tids = utils::map(m_map_tid_to_group).get_all_key();

			//归还每个线程的内存块
			for(const auto tid : tids)
			{
				//查找所在组
				auto & group = *utils::map(m_map_tid_to_group).get_existed_value(tid);
				
				//简单检查
				utils_assert(group.m_tid == tid);
				
				//如果syscall还没有返回，则不再等待
				if (group.m_is_wait_return)
					stop_wait(group, StopWaitReason::shut_down);

				//归还内存块
				give_back(group);
			}

			//简单检查: m_map_tid_to_group为空
			utils_assert(m_map_tid_to_group.empty());

			//简单检查: m_all_group里一个未用
			for(const auto & one : m_all_group)
			{
				utils_assert(!one.m_syscall);
				utils_assert(!one.m_tid);
				utils_assert(!one.m_is_wait_return);
			}
			
			//删除退出断点
			{
				//简单检查
				utils_assert(!m_all_exit_breakpoint.empty());

				//删除
				m_basic_debugger->remove_breakpoints(m_all_exit_breakpoint);
				m_all_exit_breakpoint.clear();
			}

			//释放内存
			{
				//简单检查
				utils_assert(m_buffer_addr);

				//释放内存
				utils_assert(VirtualFreeEx(m_basic_debugger->m_pi.hProcess, (LPVOID)m_buffer_addr, 0,
					MEM_RELEASE));
				m_buffer_addr = 0;

				//更新状态
				m_all_group.clear();
			}

			//更新状态
			m_is_remote_ready = false;
		}
		const std::u8string & get_syscall_name(uint64_t addr)
		{
			return utils::map(m_all_syscall).get_existed_value(addr).m_name;
		}
	private:
		//启动本地系统
		void start_up_local()
		{
			//简单检查
			utils_assert(!m_is_local_ready);

			//获取ntdll.dll模块
			m_module = &m_basic_debugger->m_module_info_manager.
				get_module_by_full_name(u8R"(C:\Windows\System32\ntdll.dll)");

			//检查: ntdll.dll不该被卸载
			m_basic_debugger->m_unload_dll_event += [this]()
			{
				auto & u = m_basic_debugger->m_debugEvent.u.UnloadDll;

				//简单检查
				utils_assert((uint64_t)u.lpBaseOfDll != m_module->m_base);
			};
			
			//获取所有syscall
			m_all_syscall = [&]()
			{
				auto export_table = m_module->m_pe_viewer->get_export_table_detail();
				std::map<uint64_t/*syscall的地址*/, std::vector<std::u8string>> export_map;
				for(const auto & one : export_table)
				{
					//简单检查
					utils_assert(!one.m_is_forward);

					export_map[one.m_rva + m_module->m_base].push_back(one.m_name);
				}
				std::map<uint64_t, s_syscall> all_syscall;
				for(const auto & one : export_map)
				{
					//检查指定位置是否有syscall指令
					auto export_func = one.first;
					uint16_t value{};
					m_basic_debugger->read_debuggee_memory(value, export_func + 0x12);
					if (0x050f != value)
						continue;

					//简单检查
					utils_assert(one.second.size() >= 2);
					utils_assert(one.second[0][0] == u8'N');
					utils_assert(one.second[0][1] == u8't');
					utils_assert(one.second[one.second.size()-1][0] == u8'Z');
					utils_assert(one.second[one.second.size()-1][1] == u8'w');
					utils_assert(one.second[0].substr(2) == one.second[one.second.size()-1].substr(2));
					
					//这个有时出现的太多了，暂时屏蔽掉
					if (one.second[0] == u8"NtYieldExecution") continue;

					const auto addr = export_func + 0x12;
					all_syscall[addr] = s_syscall{addr, one.second[0]};
				}
				return all_syscall;
			}();

			//添加处理: 当线程结束时，归还该线程占用的内存块
			auto func = [this]()
			{
				//当前线程
				auto & thread = m_basic_debugger->get_current_thread();

				//此线程从未调用过syscall
				if (!utils::map(m_map_tid_to_group).is_exist(thread.m_tid)) return;

				//查找内存块
				auto & group = *utils::map(m_map_tid_to_group).get_existed_value(thread.m_tid);

				//如果syscall还没有返回，则不再等待
				if (group.m_is_wait_return)
					stop_wait(group, StopWaitReason::exit_thread);

				//归还内存块
				give_back(group);
			};
			m_basic_debugger->m_exit_thread_event += func;
			m_basic_debugger->m_exit_process_event += func;

			//添加处理: 当调试器结束时，检查自身状态
			m_basic_debugger->m_before_exit += [this]()
			{
				check_before_exit();
			};

			//为未返回的syscall创建记录空间
			m_all_syscall_no_return.resize((uint64_t)StopWaitReason::EnumSize);

			//更新状态
			m_is_local_ready = true;
		}
		//启动远程系统
		void start_up_remote()
		{
			//简单检查
			utils_assert(!m_is_remote_ready);

			//开辟空间
			{
				//简单检查
				utils_assert(!m_buffer_addr);

				//申请空间
				m_buffer_addr = (uint64_t)VirtualAllocEx(m_basic_debugger->m_pi.hProcess,
					NULL, 4*m_group_n, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

				//简单检查
				utils_assert(m_buffer_addr);
						
				//分配内存给每个组
				m_all_group.resize(m_group_n);
				for(const auto i : utils::iterator_index(m_group_n))
				{
					m_all_group[i].m_addr = m_buffer_addr + i*4;
				}

				//写入代码
				utils::data data;
				data.resize(4*m_group_n);
				for(const auto i : utils::iterator_index(m_group_n))
				{
					data[0+i*4] = 0x0f;
					data[1+i*4] = 0x05;
					data[2+i*4] = 0xcc;
					data[3+i*4] = 0xcc;
				}
				m_basic_debugger->write_debuggee_memory(data, m_buffer_addr);
			}

			//添加退出断点
			{
				//准备断点地址
				std::vector<uint64_t> addrs;
				for(const auto & one : m_all_group)
				{
					addrs.push_back(one.m_addr + 2 /*此处有个0xcc*/);
				}

				//类型声明
				using ContinueMethod = utils::windows::debug::debugger::basic::
					DebuggerModule_Breakpoints::ContinueMethod;

				//简单检查
				utils_assert(m_all_exit_breakpoint.empty());

				//向所有地址添加断点
				m_all_exit_breakpoint = m_basic_debugger->add_breakpoints(addrs, 
					ContinueMethod::have_been_moved, [this]()
					{
						//执行ntdll.dll中syscall后面的代码
						on_exit_breakpoint();
					});
			}
			
			//更新状态
			m_is_remote_ready = true;
		}
		//不再等待syscall的返回
		void stop_wait(s_group & group, StopWaitReason stop_wait_reason)
		{
			//简单检查
			utils_assert(group.m_is_wait_return);
			utils_assert(group.m_syscall);

			//记录那些没有返回的syscall
			m_all_syscall_no_return[(uint64_t)stop_wait_reason].insert(group.m_syscall->m_name);
			
			//通知用户
			m_meet_syscall_no_return(group.m_syscall->m_addr);

			//更新状态
			group.m_is_wait_return = false;
			group.m_syscall = nullptr;
		}
		//当遇见进入断点时
		void on_enter_breakpoint(std::function<void()> meet_one_syscall)
		{
			//发出通知
			meet_one_syscall();
					
			//查找当前线程
			auto & thread = m_basic_debugger->get_current_thread();

			//获得一个可用的内存块，并设置好group.m_tid
			auto & group = [&]() -> s_group &
			{
				//如果当前线程已经在使用一个内存块, 则直接使用这个内存块
				if(utils::map(m_map_tid_to_group).is_exist(thread.m_tid))
				{
					s_group & group = *utils::map(m_map_tid_to_group).get_existed_value(thread.m_tid);

					//如果syscall还没有返回，则不再等待
					if (group.m_is_wait_return)
						stop_wait(group, StopWaitReason::another_syscall);

					//简单检查
					utils_assert(group.m_tid == thread.m_tid);

					//给出
					return group;
				}

				//否则查找一个空闲的内存块
				for(auto & group : m_all_group)
				{
					if(group.m_tid) continue;

					//标记为当前线程使用
					group.m_tid = thread.m_tid;

					//同步增加一个映射, 方便查找
					utils::map(m_map_tid_to_group).insert_not_exist(group.m_tid.value(), &group);
					
					//简单检查
					utils_assert(!group.m_syscall);

					//给出
					return group;
				}

				//没有空闲内存块
				utils_throw();
			}();

			//简单检查
			utils_assert(group.m_addr);

			//移动到每个线程自己的syscall上
			utils::windows::debugger::set_rip(thread.m_hThread, group.m_addr);

			//查找当前syscall
			auto & u = m_basic_debugger->m_debugEvent.u.Exception;
			const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;
			const auto & syscall = utils::map(m_all_syscall).get_existed_value(addr);

			//更新状态
			group.m_syscall = &syscall;
			group.m_is_wait_return = true;
		}
		//当遇见退出断点时
		void on_exit_breakpoint()
		{
			//查找当前线程
			auto & thread = m_basic_debugger->get_current_thread();

			//查找当前内存块
			auto & group = *utils::map(m_map_tid_to_group).get_existed_value(thread.m_tid);

			//当前断点位置
			auto & u = m_basic_debugger->m_debugEvent.u.Exception;
			const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;

			//简单检查
			utils_assert(group.m_addr + 2 == addr);
			utils_assert(group.m_tid == thread.m_tid);
			utils_assert(group.m_is_wait_return);

			//通知用户
			m_meet_syscall_return(group.m_syscall->m_addr);
			
			//移动回nt_call
			utils::windows::debugger::set_rip(thread.m_hThread, group.m_syscall->m_addr + 2);

			//更新状态
			group.m_syscall = nullptr;
			group.m_is_wait_return = false;
		}
		//归还内存块
		void give_back(s_group & group)
		{
			//简单检查
			utils_assert(group.m_tid);
			utils_assert(!group.m_syscall);
			utils_assert(!group.m_is_wait_return);

			//临时保存
			const auto tid = group.m_tid.value();

			//不再占用此内存块
			group.m_tid.reset();

			//同步更新map
			utils::map(m_map_tid_to_group).remove(tid);
		}
		//当调试器结束时，检查自身的状态，是否符合预期
		void check_before_exit()
		{
			utils_assert(m_map_tid_to_group.empty());

			if (m_is_remote_ready)
			{
				for(const auto & one : m_all_group)
				{
					utils_assert(!one.m_is_wait_return);
					utils_assert(!one.m_syscall);
					utils_assert(!one.m_tid);
				}
			}
			else
			{
				utils_assert(m_all_group.empty());
			}
		}
	};
	
	//通过模拟栈展开，来获取调用栈
	class StackUnwinder : public inner::OptionalFeature
	{
	private:
		utils::windows::unwind::StackUnwinder m_stack_unwinder;
		bool m_is_local_ready{};

	public:
		//获取调用栈
		//note: 从当前context.rip开始解析
		std::vector<uint64_t> get_call_stack()
		{
			//确认本地系统，已准备完毕
			if (!m_is_local_ready) start_up_local();

			//获得当前线程
			const auto & thread = m_basic_debugger->get_current_thread();

			//获得当前context
			const auto context = utils::windows::process::get_thread_context(thread.m_hThread,
				CONTEXT_INTEGER | CONTEXT_CONTROL);

			return m_stack_unwinder.get_call_stack(context);
		}
		//通过函数块中某个地址，获得函数入口地址
		std::optional<uint64_t> get_function_entry_point(uint64_t addr)
		{
			//确认本地系统，已准备完毕
			if (!m_is_local_ready) start_up_local();

			//转发
			return m_stack_unwinder.get_function_entry_point(addr);
		}
	private:
		//启动本地系统
		void start_up_local()
		{
			//简单检查
			utils_assert(!m_is_local_ready);

			//对解析器进行配置
			m_stack_unwinder.m_module_info_manager = &m_basic_debugger->m_module_info_manager;
			m_stack_unwinder.m_read_bytes =
				[this](uint64_t addr, void* buffer, uint64_t size)
				{
					m_basic_debugger->read_debuggee_memory_original(buffer, size, addr); 
				};

			//更新状态
			m_is_local_ready = true;
		}
	};

	//对一次函数调用的信息，进行读取
	class CallingReader : public inner::OptionalFeature
	{
	public:
		using FunctionSignature = utils::windows::debug::FunctionSignature;

		//读取出来的信息
		struct s_information
		{
			bool m_is_read_call_stack{};
			bool m_is_read_params{};
			bool m_is_read_return{};

			std::vector<uint64_t> m_call_stack;
			std::vector<uint64_t> m_params;
			std::optional<uint64_t> m_return_value;
			FunctionSignature m_function_signature;
			utils::data m_pointer_data;
			utils::data m_pointer_data_return;
		};

	public:
		//读取调用栈
		void read_call_stack(s_information & information)
		{
			//简单检查
			utils_assert(!information.m_is_read_call_stack);

			//获得可选功能
			auto & StackUnwinder = m_basic_debugger->get_optional_feature<features::StackUnwinder>();

			//获取调用栈
			information.m_call_stack = StackUnwinder.get_call_stack();

			//更新状态
			information.m_is_read_call_stack = true;
		}
		//读取参数
		void read_params(s_information & information)
		{
		}
		//读取返回值
		void read_return(s_information & information)
		{
		}
	};

	//记录ntdll.dll中syscall的调用情况，包括调用栈、参数和返回值
	class SyscallLogger : public inner::OptionalFeature
	{
	public:
		struct s_syscall
		{
			uint64_t m_addr{};
			std::u8string m_name;
			bool m_is_no_return{};
			features::CallingReader::s_information m_information;
		};
		struct s_thread
		{
			uint64_t tid{};
			std::deque<s_syscall> m_calls;
		};

	public:
		std::u8string m_text;
		std::deque<s_thread> m_all_history_thread;

	private:
		utils::Switch m_switch;
		bool m_is_local_ready{};
		bool m_is_wait_ntdll{};
		std::map<uint64_t, s_thread> m_all_alive_thread;

	public:
		//开启功能
		void turn_on()
		{
			//转发
			return turn_on(0);
		}
		//关闭功能
		void turn_off()
		{
			//确保开关状态正确
			m_switch.turn_off();

			//还在等待ntdll.dll的加载
			if (m_is_wait_ntdll)
			{
				m_is_wait_ntdll = false;
				return;
			}

			//取消syscall的断点
			m_basic_debugger->get_optional_feature<features::BreakAllSyscall>().turn_off();
		}
	private:
		//开启功能
		void turn_on(uint64_t stage)
		{
			switch(stage)
			{
			case 0:
			{
				//确保开关状态正确
				m_switch.turn_on();

				//确认本地系统，已准备完毕
				if (!m_is_local_ready) start_up_local();

				//简单检查
				utils_assert(!m_is_wait_ntdll);

				//ntdll.dll还未加载
				if(!m_basic_debugger->m_module_info_manager.is_exist_by_full_name(
					u8R"(C:\Windows\System32\ntdll.dll)"))
				{
					m_is_wait_ntdll = true;
				}
				//否则，进行下一阶段
				else
				{
					turn_on(1);
				}
				return;
			}
			//此时ntdll已经加载
			case 1:
			{
				//简单检查
				utils_assert(!m_is_wait_ntdll);

				//简单检查
				utils_assert(m_basic_debugger->m_module_info_manager.is_exist_by_full_name(
					u8R"(C:\Windows\System32\ntdll.dll)"));

				//给syscall下断点
				m_basic_debugger->get_optional_feature<features::BreakAllSyscall>().turn_on(
					[this]() { return on_meet_syscall_call(); },
					[this](uint64_t addr) { return on_meet_syscall_return(addr); },
					[this](uint64_t addr) { return on_meet_syscall_no_return(addr); });
				return;
			}
			}
			utils_throw();
		}
		//syscall被调用
		void on_meet_syscall_call()
		{
			//断点地址
			const auto & u = m_basic_debugger->m_debugEvent.u.Exception;
			const auto addr = (uint64_t)u.ExceptionRecord.ExceptionAddress;
			
			//查找可选功能
			auto & BreakAllSyscall = m_basic_debugger->get_optional_feature<features::BreakAllSyscall>();

			//获得断点名称
			const auto & name = BreakAllSyscall.get_syscall_name(addr);

			//新建一条记录
			s_syscall syscall{};

			//填充基本信息
			syscall.m_addr = addr;
			syscall.m_name = name;

			//查找可选功能
			auto & CallingReader = m_basic_debugger->get_optional_feature<features::CallingReader>();

			//读取调用栈
			CallingReader.read_call_stack(syscall.m_information);

			//读取参数
			//CallingReader.read_params(syscall.m_information, get_function_signature(name));
			CallingReader.read_params(syscall.m_information);

			//保存
			save_new_syscall(std::move(syscall));

			//其他待删
			m_text += name;
			m_text += u8"\n";
		}
		//保存一条新建记录
		void save_new_syscall(s_syscall && syscall)
		{
			//获得当前线程
			auto & thread = m_basic_debugger->get_current_thread();

			//如果是当前线程的第一条记录，则创建当前线程的条目
			if (!utils::map(m_all_alive_thread).is_exist(thread.m_tid))
			{
				m_all_alive_thread[thread.m_tid].tid = thread.m_tid;
			}

			//获得当前线程的记录
			auto & alive_thread = utils::map(m_all_alive_thread).get_existed_value(thread.m_tid);

			//添加记录
			alive_thread.m_calls.push_back(std::move(syscall));
		}
		//syscall已返回
		void on_meet_syscall_return(uint64_t addr)
		{
			//获得当前线程
			auto & thread = m_basic_debugger->get_current_thread();

			//获得当前线程的记录
			auto & alive_thread = utils::map(m_all_alive_thread).get_existed_value(thread.m_tid);

			//获得当前线程最后一条记录
			auto & last_one = utils::deque(alive_thread.m_calls).last_one();

			//查找可选功能
			auto & CallingReader = m_basic_debugger->get_optional_feature<features::CallingReader>();

			//读取返回值
			CallingReader.read_return(last_one.m_information);

			//其他待删
			auto & BreakAllSyscall = m_basic_debugger->get_optional_feature<features::BreakAllSyscall>();
			const auto & name = BreakAllSyscall.get_syscall_name(addr);
			m_text += name;
			m_text += u8" return\n";
		}
		//syscall未返回
		void on_meet_syscall_no_return(uint64_t addr)
		{
			//获得当前线程
			auto & thread = m_basic_debugger->get_current_thread();

			//获得当前线程的记录
			auto & alive_thread = utils::map(m_all_alive_thread).get_existed_value(thread.m_tid);

			//获得当前线程最后一条记录
			auto & last_one = utils::deque(alive_thread.m_calls).last_one();
			
			//给出信息
			last_one.m_is_no_return = true;
			
			//其他待删
			auto & BreakAllSyscall = m_basic_debugger->get_optional_feature<features::BreakAllSyscall>();
			const auto & name = BreakAllSyscall.get_syscall_name(addr);
			m_text += name;
			m_text += u8" no return\n";
		}
		//启动本地系统
		void start_up_local()
		{
			//简单检查
			utils_assert(!m_is_local_ready);

			//添加处理: 当ntdll.dll载入时，根据设定开启功能
			m_basic_debugger->m_load_dll_event += [this]()
			{
				const auto & u = m_basic_debugger->m_debugEvent.u.LoadDll;
				const auto full_name = utils::windows::file::get_full_name_by_hFile(u.hFile);

				if(utils::u8string(full_name).view().compare_is_same(u8R"(C:\Windows\System32\ntdll.dll)", true))
				{
					if(m_is_wait_ntdll)
					{
						m_is_wait_ntdll = false;

						turn_on(1);
					}
				}
			};

			//添加处理: 当线程终止时，把当前线程的记录移动到历史线程里
			auto on_exit_thread_event = [this]()
			{
				//获得当前线程
				auto & thread = m_basic_debugger->get_current_thread();

				//获得当前线程的记录
				auto & alive_thread = utils::map(m_all_alive_thread).get_existed_value(thread.m_tid);

				//移动到历史线程里
				m_all_history_thread.push_back(std::move(alive_thread));
				
				//删除当前线程的记录
				utils::map(m_all_alive_thread).remove(thread.m_tid);
			};
			m_basic_debugger->m_exit_thread_event += on_exit_thread_event;
			m_basic_debugger->m_exit_process_event += on_exit_thread_event;

			//添加处理: 当调试器终止时，检查状态
			m_basic_debugger->m_before_exit += [this]()
			{
				return check_before_exit();
			};

			//更新状态
			m_is_local_ready = true;
		}
		//调试器结束时，检查自身状态
		void check_before_exit()
		{
			utils_assert(m_all_alive_thread.empty());
		}
#if 0
		//获得函数签名
		//name: 函数名
		std::optional<utils::windows::debug::FunctionSignature> get_function_signature(const std::u8string & name)
		{
			static const std::map<std::u8string, utils::windows::debug::FunctionSignature> all_signature{
				{u8"NtCreateFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtOpenFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtReadFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtWriteFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtClose", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtQueryInformationFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtSetInformationFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtQueryDirectoryFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtQueryVolumeInformationFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtSetVolumeInformationFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtFsControlFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
				{u8"NtDeviceIoControlFile", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
			};
			if (utils::map(all_signature).is_exist(name))
				return utils::map(all_signature).get_existed_value(name);
			else
				return {};
		}
#endif
	};

	//加载所有可选功能的调试器
	class FullDebugger : public utils::windows::debug::debugger::basic::BasicDebugger
	{
	public:
		FullDebugger()
		{
			add_optional_feature<BreakAllSyscall>();
			add_optional_feature<StackUnwinder>();
			add_optional_feature<CallingReader>();
			add_optional_feature<SyscallLogger>();
		}
	};
}
namespace utils::windows::debug::debugger
{
	using FullDebugger = utils::windows::debug::debugger::features::FullDebugger;
}