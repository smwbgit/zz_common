#pragma once

#include <windows.h>
#include <functional>
#include <vector>
#include <dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")
#include <utils/assert.hpp>

namespace utils::windows::symbol
{
	//对给定地址，查找符号
	//note: 使用_NT_SYMBOL_PATH环境变量
	class ProcessSymbol
	{
	private:
		HANDLE m_hProcess{};
	public:
		//is_load_lines: 是否载入源码相关数据
		ProcessSymbol(HANDLE hProcess, bool is_load_lines = false)
		{
			//简单检查
			utils_assert(hProcess != GetCurrentProcess());	//此时生成一个用static记录的id值，比如，1，2，3，4，etc

			m_hProcess = hProcess;
			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_EXACT_SYMBOLS
				| (is_load_lines?SYMOPT_LOAD_LINES:0));
			utils_assert(SymInitialize(m_handle, NULL, TRUE));
		}
		~SymbolDatabase()
		{
			utils_assert_abort(SymCleanup(m_hProcess));
		}
		//return: 是否找到
		//info: 找到的信息
		bool get_symbol(uint64_t addr, std::function<void(const SYMBOL_INFO & info)> meet_symbol)
		{
			const uint64_t max_len = 100;
			std::vector<uint8_t> buffer;
			buffer.resize(sizeof(SYMBOL_INFO) + max_len + 2);
			SYMBOL_INFO & info = *(SYMBOL_INFO*)&buffer[0];
			info.SizeOfStruct = sizeof(SYMBOL_INFO);
			info.MaxNameLen = max_len;

			if (SymFromAddr(m_handle, addr, 0, &info))
			{
				//简单检查
				utils_assert(info.Flags & SYMFLAG_FUNCTION);

				meet_symbol(info);
				return true;
			}
			else
				return false;
		}
	};
}