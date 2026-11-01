#pragma once

#include <windows.h>
#include <string>
#include <utils/assert.hpp>
#include <utils/windows/handle/ManagedHandle.hpp>

namespace utils::windows::module
{
	template<typename function_T>
	function_T get_function_address(const std::wstring & dll_name, const std::string & function_name)
	{
		HMODULE _hDll{};
		utils_assert(GetModuleHandleExW(0, dll_name.c_str(), &_hDll));
		utils_assert(_hDll);
		utils::windows::handle::ManagedHandle<HMODULE> hDll;
		hDll.set(_hDll);

		const auto function = (function_T)GetProcAddress(hDll, function_name.c_str());
		utils_assert(function);

		return function;
	}
}