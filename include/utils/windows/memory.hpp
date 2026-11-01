#pragma once

#include <windows.h>
#include <optional>
#include <filesystem>
#include <utils/assert.hpp>
#include <utils/data.hpp>
#include <utils/windows/Handle.hpp>
#include <utils/FileWriter.hpp>
#include <utils/windows/exception.hpp>

//内存相关
namespace utils::windows::memory
{
	//对VirtualQuery的简单包装
	//return: 当地址超出最大可用地址时，返回空
	inline std::optional<MEMORY_BASIC_INFORMATION> query(HANDLE hProcess, uint64_t addr)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		auto re = VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi));
		if (re == 0)
		{
			utils_assert(GetLastError() == ERROR_INVALID_PARAMETER);
			return {};
		}
		utils_assert(sizeof(mbi) == re);
		return mbi;
	}
	//特化: 返回所有涉及到的内存块，如果其中有不能获取的，则报错
	inline std::vector<MEMORY_BASIC_INFORMATION> query(HANDLE hProcess, uint64_t addr, uint64_t size)
	{
		std::vector<MEMORY_BASIC_INFORMATION> mbis;
		uint64_t cur = addr;
		while(cur < addr + size)
		{
			auto mbi = query(hProcess, cur);

			//简单检查
			utils_assert(mbi.has_value());
			utils_assert((uint64_t)mbi->BaseAddress <= cur);
			utils_assert((uint64_t)mbi->BaseAddress + mbi->RegionSize > cur);

			cur = (uint64_t)mbi->BaseAddress + mbi->RegionSize;

			mbis.push_back(std::move(mbi.value()));
		}
		return mbis;
	}
	//所处内存页是否已提交
	inline bool is_commit(const MEMORY_BASIC_INFORMATION & mbi)
	{
		return mbi.State == MEM_COMMIT;
	}
	//所处内存页是否为guard页面
	inline bool is_guard(const MEMORY_BASIC_INFORMATION & mbi)
	{
		return mbi.Protect & PAGE_GUARD;
	}
	//所处内存页是否可读
	inline bool is_readable(const MEMORY_BASIC_INFORMATION & mbi)
	{
		if (!is_commit(mbi)) return false;
		if (is_guard(mbi)) return false;
		return mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY) ? true : false;
	}
	//特化
	inline bool is_readable(HANDLE hProcess, uint64_t addr)
	{
		auto mbi = query(hProcess, addr);
		if (!mbi.has_value()) return false;

		return is_readable(mbi.value());
	}
	//所处内存页是否可以执行
	inline bool is_executable(const MEMORY_BASIC_INFORMATION & mbi)
	{
		if (!is_commit(mbi)) return false;
		if (is_guard(mbi)) return false;
		return mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY) ? true : false;
	}
	//特化
	inline bool is_executable(HANDLE hProcess, uint64_t addr)
	{
		auto mbi = query(hProcess, addr);
		if (!mbi.has_value()) return false;

		return is_executable(mbi.value());
	}
	//一个内存块
	struct s_memory_block
	{
		MEMORY_BASIC_INFORMATION info{};
	};
	//返回指定进程的内存块列表
	inline std::vector<s_memory_block> get_memory_blocks(DWORD pid = GetCurrentProcessId())
	{
		SYSTEM_INFO si{};
		GetSystemInfo(&si);
		const auto min_addr = si.lpMinimumApplicationAddress;
		const auto max_addr = si.lpMaximumApplicationAddress;
		const auto step = si.dwAllocationGranularity;

		utils::windows::Handle process{OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid), NULL};

		std::vector<s_memory_block> blocks;
		for(auto p = min_addr; p < max_addr; )
		{
			MEMORY_BASIC_INFORMATION info{};
			auto re = VirtualQueryEx(process, p, &info, sizeof(info));
			if(re == 0)
			{
				p = (char*)p + step;
				continue;
			}
			utils_assert(re == sizeof(info));

			blocks.push_back({info});
			p = (char*)info.BaseAddress + info.RegionSize;
		}
		return blocks;
	}
	//读取指定进程的内存
	inline void read_memory(HANDLE hProcess, uint64_t addr, void* buffer, uint64_t size)
	{
		SIZE_T NumberOfBytesRead{};
		utils_assert(ReadProcessMemory(hProcess, (void*)addr, buffer, size, &NumberOfBytesRead));
		utils_assert(NumberOfBytesRead == size);
	}
	//特化
	inline utils::data read_memory(HANDLE hProcess, uint64_t addr, uint64_t size)
	{
		utils::data data;
		data.resize(size);
		read_memory(hProcess, addr, data.addr(), size);
		return data;
	}
	//特化
	inline utils::data read_memory(DWORD pid, uint64_t addr, uint64_t size)
	{
		utils::windows::Handle process{OpenProcess(PROCESS_VM_READ, FALSE, pid), NULL};
		return read_memory(process, addr, size);
	}
	//写入指定进程的内存
	inline void write_memory(HANDLE hProcess, uint64_t addr, utils::data_view data_view)
	{
		SIZE_T NumberOfBytesWritten{};
		utils_assert(WriteProcessMemory(hProcess, (void*)addr, data_view.addr(), data_view.size(), &NumberOfBytesWritten));
		utils_assert(NumberOfBytesWritten == data_view.size());
	}
	namespace inner
	{
		//临时修改内存的保护属性，变的可读或可写
		inline void change_to_xxxxable_and_restore(HANDLE hProcess, uint64_t addr, uint64_t size, std::function<void()> func, std::function<DWORD(const MEMORY_BASIC_INFORMATION & mbi)> get_new)
		{
			const auto mbis = query(hProcess, addr, size);

			std::vector<DWORD> news;
			for(const auto & mbi : mbis)
				news.push_back(get_new(mbi));
			std::vector<DWORD> olds; olds.resize(mbis.size());

			for(const auto i : utils::iterator_index(mbis))
			{
				if(news[i] != mbis[i].Protect)
				{
					utils_assert(VirtualProtectEx(hProcess, mbis[i].BaseAddress, mbis[i].RegionSize,
						news[i], &olds[i]));
				}
			}
			func();
			for(const auto i : utils::iterator_index(mbis))
			{
				if(news[i] != mbis[i].Protect)
				{
					utils_assert(VirtualProtectEx(hProcess, mbis[i].BaseAddress, mbis[i].RegionSize,
						olds[i], &olds[i]));
				}
			}
		}
	}
	//临时修改内存的保护属性，变的可读
	inline void change_to_readable_and_restore(HANDLE hProcess, uint64_t addr, uint64_t size, std::function<void()> func)
	{
		return inner::change_to_xxxxable_and_restore(hProcess, addr, size, func, [](const MEMORY_BASIC_INFORMATION & mbi) -> DWORD
		{
			//简单检查
			utils_assert(utils::windows::memory::is_commit(mbi));
			utils_assert(!utils::windows::memory::is_guard(mbi));

			if (mbi.Protect & PAGE_NOACCESS)			return mbi.Protect & (~PAGE_NOACCESS) | PAGE_READONLY;
			if (mbi.Protect & PAGE_READONLY)			return mbi.Protect;
			if (mbi.Protect & PAGE_READWRITE)			return mbi.Protect;
			if (mbi.Protect & PAGE_WRITECOPY)			return mbi.Protect;
			if (mbi.Protect & PAGE_EXECUTE)				return mbi.Protect & (~PAGE_EXECUTE) | PAGE_EXECUTE_READ;
			if (mbi.Protect & PAGE_EXECUTE_READ)		return mbi.Protect;
			if (mbi.Protect & PAGE_EXECUTE_READWRITE)	return mbi.Protect;
			if (mbi.Protect & PAGE_EXECUTE_WRITECOPY)	return mbi.Protect;
			utils_throw();
		});
	}
	//临时修改内存的保护属性, 变的可写
	inline void change_to_writable_and_restore(HANDLE hProcess, uint64_t addr, uint64_t size, std::function<void()> func)
	{
		return inner::change_to_xxxxable_and_restore(hProcess, addr, size, func, [](const MEMORY_BASIC_INFORMATION & mbi) -> DWORD
		{
			//简单检查
			utils_assert(utils::windows::memory::is_commit(mbi));
			utils_assert(!utils::windows::memory::is_guard(mbi));
			
			if (mbi.Protect & PAGE_NOACCESS)			return mbi.Protect & (~PAGE_NOACCESS) | PAGE_READWRITE;
			if (mbi.Protect & PAGE_READONLY)			return mbi.Protect & (~PAGE_READONLY) | PAGE_READWRITE;
			if (mbi.Protect & PAGE_READWRITE)			return mbi.Protect;
			if (mbi.Protect & PAGE_WRITECOPY)			utils_throw();	//return mbi.Protect;	//todo 看看这里如何处理
			if (mbi.Protect & PAGE_EXECUTE)				return mbi.Protect & (~PAGE_EXECUTE) | PAGE_EXECUTE_READWRITE;
			if (mbi.Protect & PAGE_EXECUTE_READ)		return mbi.Protect & (~PAGE_EXECUTE_READ) | PAGE_EXECUTE_READWRITE;
			if (mbi.Protect & PAGE_EXECUTE_READWRITE)	return mbi.Protect;
			if (mbi.Protect & PAGE_EXECUTE_WRITECOPY)	utils_throw();	//return mbi.Protect;	//todo 看看这里如何处理
			utils_throw();
		});
	}
	//写入可能不能写的内存
	inline void write_protected_memory(HANDLE hProcess, uint64_t addr, utils::data_view data_view)
	{
		utils::windows::memory::change_to_writable_and_restore(hProcess, addr, data_view.size(), [&]()
		{
			utils::windows::memory::write_memory(hProcess, addr, data_view);
		});
	}
	//读取可能不能读的内存
	inline void read_protected_memory(HANDLE hProcess, uint64_t addr, void* buffer, uint64_t size)
	{
		utils::windows::memory::change_to_readable_and_restore(hProcess, addr, size, [&]()
		{
			utils::windows::memory::read_memory(hProcess, addr, buffer, size);
		});
	}
	//向前尽量读取, 直到内存页属性为 "is_readable()==false"
	inline utils::data read_greed_back(HANDLE hProcess, uint64_t addr, uint64_t size)
	{
		SYSTEM_INFO si{}; GetSystemInfo(&si);
		const uint64_t page_size = si.dwPageSize;

		auto is_readable = [hProcess](uint64_t addr)
		{
			return utils::windows::memory::is_readable(hProcess, addr);
		};
		auto readable_size = [=, &is_readable]()
		{
			uint64_t _addr = addr;
			uint64_t _size = size;
			while(1)
			{
				if (!is_readable(_addr))
					break;

				const auto size_in_page = (_addr&(page_size-1))+1;
				if (size_in_page >= _size)
				{
					_size = 0; break;
				}

				_addr -= size_in_page;
				_size -= size_in_page;
			}
			return size - _size;
		}();

		utils::data data;
		if (utils::windows::exception::catch_memory_access_exception([&]()
		{
			data.assign((const void*)(addr-readable_size+1), readable_size);
		}))
		{
			return {};
		}

		return data;
	}
}
