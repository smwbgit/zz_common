#pragma once

#include <windows.h>
#include <string>
#include <optional>
#include <tlhelp32.h>
#include <utils/assert.hpp>
#include <utils/windows/Handle.hpp>
#include <utils/windows/pipe.hpp>
#include <utils/windows/memory.hpp>
#include <utils/windows/file.hpp>
#include <utils/MoreData.hpp>
#include <utils/windows/pe.hpp>
#include <utils/functions.hpp>
#include <utils/Range1D.hpp>
#include <utils/windows/file/FileMapping.hpp>


//进程
namespace utils::windows::process
{
#if 0	//用哪个恢复哪个
	//等待指定进程退出
	inline void wait_exit_code(HANDLE process)
	{
		auto re = WaitForSingleObject(process, INFINITE);

		//简单检查
		utils_assert(re == WAIT_OBJECT_0);
	}

	//检查指定进程是否在运行
	inline bool is_running(HANDLE process)
	{
		auto re = WaitForSingleObject(process, 0);
		if (re == WAIT_TIMEOUT)
			return true;
		if (re == WAIT_OBJECT_0)
			return false;
		utils_throw();
	}

	//用指定的输入, 运行指定的程序, 返回程序的输出和退出码
	//command: 要运行的程序以及参数
	inline void run(
		_Out_ uint32_t & exit_code,
		_Out_ std::string & output,
		_In_ const std::string & command,
		_In_ const std::string & input)
	{
		utils::windows::api::handle handle_stdin_read;
		utils::windows::api::handle handle_stdin_write;
		utils::windows::api::handle handle_stdout_read;
		utils::windows::api::handle handle_stdout_write;
		{
			SECURITY_ATTRIBUTES sa{};
			{
				sa.nLength = sizeof(sa);
				sa.bInheritHandle = TRUE;
				sa.lpSecurityDescriptor = NULL;
			}
			{
				HANDLE handle_read{};
				HANDLE handle_write{};
				utils_assert(CreatePipe(&handle_read, &handle_write, &sa, 0));
				utils_assert(SetHandleInformation(handle_write, HANDLE_FLAG_INHERIT, 0));
				handle_stdin_read = handle_read;
				handle_stdin_write = handle_write;
			}
			{
				HANDLE handle_read{};
				HANDLE handle_write{};
				utils_assert(CreatePipe(&handle_read, &handle_write, &sa, 0));
				utils_assert(SetHandleInformation(handle_read, HANDLE_FLAG_INHERIT, 0));
				handle_stdout_read = handle_read;
				handle_stdout_write = handle_write;
			}
		}

		//todo: 看看能不能把其他所有已创建的句柄都遍历一遍,检查下有没有可继承的,有的话关闭掉再创建子进程,或者有的话就报错
		//todo: (cont.) UpdateProcThreadAttribute

		STARTUPINFOA si{};
		{
			si.cb = sizeof(si);
			si.hStdInput = handle_stdin_read;
			si.hStdOutput = handle_stdout_write;
			si.hStdError = handle_stdout_write;
			si.dwFlags |= STARTF_USESTDHANDLES;
		}

		PROCESS_INFORMATION pi{};
		utils_assert(CreateProcessA(NULL, (char*)command.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi));

		CloseHandle(pi.hThread);
		utils::windows::api::handle handle_process = pi.hProcess;

		//写入
		{
			DWORD written_size{};
			utils_assert(WriteFile(handle_stdin_write, input.c_str(), (DWORD)input.length(), &written_size, NULL));	//todo 第三个参数强转检查是否超出DWORD
			utils_assert(written_size == input.length());
		}

		//读取
		{
			//函数: 做一次读取
			auto do_read = [&]()
			{
				while (1)
				{
					//如果没有数据,则返回
					if (false == utils::windows::api::pipe::is_data_reached(handle_stdout_read))
						return;

					//读取
					char buffer[1024]{};
					DWORD readed_size{};
					auto is_ok = ReadFile(handle_stdout_read, buffer, sizeof(buffer), &readed_size, NULL);

					//简单检查
					utils_assert(is_ok);
					utils_assert(readed_size != 0);

					//保存
					output.insert(output.end(), buffer, buffer + readed_size);
				}
			};

			HANDLE handles[]{handle_process, handle_stdout_read};

			while(1)
			{
				DWORD re = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, FALSE);

				if (re == WAIT_OBJECT_0 + 1)
				{
					do_read();
					continue;
				}

				//简单检查
				utils_assert(re == WAIT_OBJECT_0);

				//进程退出了, 看看还有没有数据了
				do_read();
				break;
			}
		}

		//简单检查
		utils_assert(utils::windows::process::is_running(handle_process) == false);

		utils_assert(GetExitCodeProcess(handle_process, (LPDWORD)&exit_code));
	}

	//获得指定进程的所有tid
	inline std::vector<DWORD> get_tids(DWORD pid)
	{
		utils::windows::api::handle hThreadSnap{CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), INVALID_HANDLE_VALUE};

		THREADENTRY32 te32{};
		te32.dwSize = sizeof(te32);

		utils_assert(Thread32First(hThreadSnap, &te32));
		utils_assert(te32.dwSize == sizeof(te32));

		std::vector<DWORD> tids;
		do
		{
			if (te32.th32OwnerProcessID == pid)
				tids.push_back(te32.th32ThreadID);
		} while (Thread32Next(hThreadSnap, &te32));

		return tids;
	}
	
	//对OpenThread的简单包装
	inline utils::windows::api::handle open_thread(DWORD tid, DWORD desired_access = THREAD_ALL_ACCESS)
	{
		return {OpenThread(desired_access, FALSE, tid), NULL};
	}

	//临时冻结一个进程
	inline void temporary_freeze_process(std::function<void()> func, DWORD pid = GetCurrentProcessId())
	{
		auto tids = utils::windows::process::get_tids(pid);
		
		std::vector<utils::windows::api::handle> threads;
		for(const auto tid : tids)
		{
			if(pid == GetCurrentProcessId() && tid == GetCurrentThreadId()) continue;

			auto thread = utils::windows::process::open_thread(tid, THREAD_SUSPEND_RESUME);
			utils_assert((DWORD)-1 != SuspendThread(thread) || GetLastError() == ERROR_ACCESS_DENIED);	//todo 看看这些线程,为什么拒绝访问
			threads.push_back(std::move(thread));
		}
		
		//todo 再检查一遍，看看还有没有没挂起的，重复，直到都挂起了

		func();
		
		for(auto & thread : threads)
			utils_assert((DWORD)-1 != ResumeThread(thread) || GetLastError() == ERROR_ACCESS_DENIED);
	}

	//对GetThreadContext的简单包装
	inline CONTEXT get_thread_context(DWORD tid, DWORD context_flags = CONTEXT_CONTROL)
	{
		auto thread = utils::windows::process::open_thread(tid, THREAD_GET_CONTEXT);
		CONTEXT context{};
		context.ContextFlags = context_flags;
		utils_assert(GetThreadContext(thread, &context));
		return context;
	}
#endif
	namespace inner::get_all_module
	{
		struct s_all_module
		{
		public:
			struct s_module
			{
				std::u8string base_name;
				std::u8string full_name;
				uint64_t base{};
				uint64_t size{};
			};
		public:
			std::vector<s_module> m_modules;
		public:
			const s_module & get_module_by_full_name(const std::u8string & full_name)
			{
				for (const auto & one : m_modules)
				{
					if (utils::u8string(full_name).view().compare_is_same(one.full_name, true))
						return one;
				}
				utils_throw();
			}
			std::optional<const s_module*> get_module_by_inside_addr(uint64_t addr)
			{
				for (const auto & one : m_modules)
				{
					if (addr >= one.base && addr < one.base + one.size)
						return &one;
				}
				return {};
			}
		};
	}

#if 0
	//获得指定进程的所有dll的信息
	inline inner::get_all_module::s_all_module get_all_module(uint32_t pid = GetCurrentProcessId())
	{
		//ai生成后整理

	}
#endif

#pragma region 线程
	//暂停线程
	inline void suspend_thread(HANDLE hThread)
	{
		utils_assert((DWORD)-1 != SuspendThread(hThread));
	}
	//恢复线程的执行
	inline void resume_thread(HANDLE hThread)
	{
		utils_assert((DWORD)-1 != ResumeThread(hThread));
	}
	//获得线程id
	inline DWORD get_thread_id(HANDLE hThread)
	{
		DWORD tid = GetThreadId(hThread);

		//简单检查
		utils_assert(tid);

		return tid;
	}
	//ContextFlags: e.g. CONTEXT_INTEGER CONTEXT_CONTROL
	inline CONTEXT get_thread_context(HANDLE hThread, DWORD ContextFlags)
	{
		CONTEXT context{};
		context.ContextFlags = ContextFlags;
		utils_assert(GetThreadContext(hThread, &context));
		return context;
	}
#pragma endregion

#pragma region 模块
	//负责存放一些/所有dll的信息
	class ModuleInfoManager
	{
	public:
		struct s_module : public utils::MoreData
		{
			uint64_t m_base{};
			uint64_t m_size{};
			std::u8string m_name;
			std::u8string m_full_name;

			//以下2个，当m_is_load_file == true时开启
			std::unique_ptr<utils::windows::file::FileMapping> m_file_mapping;
			std::unique_ptr<utils::windows::pe::PeViewer> m_pe_viewer;
		};

		using meet_add_module_t = utils::functions<void(s_module& module)>;

		//请指定
	public:
		//每添加一个dll后，都会调用一次这个函数，以便让用户初始化m_user_data
		meet_add_module_t m_meet_add_module{};
		//是否读取文件并准备好pe_viewer
		bool m_is_load_file{};

	public:
		std::vector<std::unique_ptr<s_module>> m_modules;

		//添加和移除
	public:
		//获得指定进程的所有dll的信息
		void add_all(DWORD pid)
		{
			//ai生成后整理

			// 创建一个包含系统中所有模块的快照
			utils::windows::Handle hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

			//简单检查
			utils_assert(hModuleSnap != INVALID_HANDLE_VALUE);

			// 初始化me32的大小
			MODULEENTRY32W me32{};
			me32.dwSize = sizeof(me32);

			// 获取第一个模块的信息
			utils_assert(Module32FirstW(hModuleSnap, &me32));

			// 遍历模块列表
			do {
				add(
					(uint64_t)me32.modBaseAddr,
					me32.modBaseSize,
					utils::text::encoding::unicode::utf16_to_utf8((const char16_t*)me32.szModule),
					utils::text::encoding::unicode::utf16_to_utf8((const char16_t*)me32.szExePath)
				);
			} while (Module32NextW(hModuleSnap, &me32));

	#if 0
			uint64_t gs{};
			utils::cpu::get_gs(gs);
			auto peb = utils::basic::layout::get<uint64_t>(gs, 0x60);
			auto ldr = utils::basic::layout::get<uint64_t>(peb, 0x18);
			auto & load_order = utils::basic::layout::get<LIST_ENTRY64>(ldr, 0x10);
			inner::get_dlls::s_dlls dlls;
			for (uint64_t p = load_order.Flink; p != (uint64_t)&load_order.Flink; p = *(uint64_t*)p)
			{
				auto base = utils::basic::layout::get<uint64_t>(p, 0x30);
				auto size = utils::basic::layout::get<uint32_t>(p, 0x40);
				std::string base_name;
				utils::windows::text::from_UNICODE_STRING(base_name, p + 0x58);
				std::string full_name;
				utils::windows::text::from_UNICODE_STRING(full_name, p + 0x48);

				dlls.m_dlls.push_back({base_name,full_name,base,size});
			}
			return dlls;
	#endif
		}
		void add(uint64_t base, uint64_t size, std::u8string_view name, std::u8string_view full_name)
		{
			//简单检查
			utils_assert(size > 0);

			//简单检查: 此dll没有载入过
			for(const auto & one : m_modules)
			{
				utils_assert(!utils::Range1D(one->m_base, one->m_size).is_include(base));
				utils_assert(!utils::Range1D(one->m_base, one->m_size).is_include(base+size-1));
			}

			auto one = std::make_unique<s_module>();
			one->m_base = base;
			one->m_size = size;
			one->m_name = name;
			one->m_full_name = full_name;
			if(m_is_load_file)
			{
				one->m_file_mapping = std::make_unique<utils::windows::file::FileMapping>(
					one->m_full_name);
				one->m_pe_viewer = std::make_unique<utils::windows::pe::PeViewer>(
					one->m_file_mapping->addr(), true);
			}
			m_meet_add_module.call(*one);
			m_modules.push_back(std::move(one));
		}
		void add(HANDLE hFile, uint64_t base)
		{
			const auto full_name = utils::windows::file::get_full_name_by_hFile(hFile);

			utils::FileReader input{full_name};
			auto file_data = input.read_all();
			utils::windows::pe::PeViewer pe_viewer{file_data.addr(), true};
			const auto size = pe_viewer.get_nt_headers().OptionalHeader.SizeOfImage;

			add(base, size, utils::path::get_filename(full_name), full_name);
		}
		void remove(uint64_t base)
		{
			for(uint64_t i = 0; i < m_modules.size(); i++)
			{
				if (base == m_modules[i]->m_base)
					m_modules.erase(m_modules.begin() + i);
			}
		}

		//查询
	public:
		s_module & get_module_by_inside_addr(uint64_t addr)
		{
			for (auto & one : m_modules)
			{
				if (utils::Range1D{one->m_base, one->m_size}.is_include(addr))
					return *one;
			}
			utils_throw();
		}
		s_module & get_module_by_full_name(const std::u8string & full_name)
		{
			for (const auto & one : m_modules)
			{
				if (utils::u8string(full_name).view().compare_is_same(one->m_full_name, true))
					return *one;
			}
			utils_throw();
		}
		bool is_exist_by_full_name(const std::u8string & full_name) const
		{
			for (const auto & one : m_modules)
			{
				if (utils::u8string(full_name).view().compare_is_same(one->m_full_name, true))
					return true;
			}
			return false;
		}
	};
#pragma endregion
}
