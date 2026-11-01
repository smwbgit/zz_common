#pragma once

#include <windows.h>
#include <shellscalingapi.h>		//GetProcessDpiAwareness需要
#pragma comment(lib, "shcore.lib")	//GetProcessDpiAwareness需要
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>
#include <utils/CallWhenReturn.hpp>
#include <utils/basic.hpp>


namespace utils::windows::ui::dpi
{
	namespace inner
	{
		inline const uint64_t standard_dpi = 96;

		inline uint64_t get_current_dpi()
		{
			HDC dc = GetDC(0);
			utils_assert(dc != NULL);
			utils::CallWhenReturn clean{[&]()
			{
				ReleaseDC(0, dc);
			}};

			auto tmp = GetDeviceCaps(dc, LOGPIXELSY);
			utils_assert(tmp > 0);
			return tmp;
		}
	}

	//获得当前dpi
	//note: return 96 when 100%
	//note: return 120 when 125%
	//note: return 144 when 150%
	inline uint64_t get_current_dpi()
	{
		static uint64_t dpi = inner::get_current_dpi();

		return dpi;
	}
	//放大: 把100%缩放率的长度转为当前屏幕缩放率的长度
	//is_round_up_when_0_5: 当结果的小数部分正好是0.5时,是否进位
	template<typename T, bool is_round_up_when_0_5 = true>
	void get_scaled_size(_Out_ T & scaled_size, _In_ const T original_size)
	{
		//简单检查
		static_assert(utils::is_arithmetic_v<T> && sizeof(T) >= 2);		//不让T太小, 避免算超界

		double value = 1.0 * original_size * get_current_dpi() / inner::standard_dpi;
		utils::basic::store_this_floating_point<is_round_up_when_0_5>(scaled_size, value);
	}
	//特化
	template<typename T>
	void get_scaled_size(_Inout_ T & size)
	{
		return get_scaled_size<T>(size, size);
	}
#if 0
	//特化
	template<>
	inline void get_scaled_size<CRect>(_Inout_ CRect & rect)
	{
		get_scaled_size(rect.left);
		get_scaled_size(rect.top);
		get_scaled_size(rect.right);
		get_scaled_size(rect.bottom);
	}
#endif
	//缩小: 把当前屏幕缩放率的长度转为100%缩放率的长度
	//is_round_up_when_0_5: 当结果的小数部分正好是0.5时,是否进位
	template<typename T, bool is_round_up_when_0_5 = true>
	void get_original_size(_Out_ T & original_size, _In_ const T scaled_size)
	{
		//简单检查
		static_assert(utils::is_arithmetic_v<T> && sizeof(T) >= 2);		//不让T太小, 避免算超界

		double value = 1.0 * scaled_size * inner::standard_dpi / get_current_dpi();
		utils::basic::store_this_floating_point<is_round_up_when_0_5>(original_size, value);
	}
	//特化
	template<typename T>
	void get_original_size(_Inout_ T & size)
	{
		return get_original_size<T>(size, size);
	}

	
    //对GetProcessDpiAwareness的简单包装
    /*
    example:
	    const char* mode{};
        switch (utils::windows::ui::dpi::get_process_dpi_awareness()) {
            case PROCESS_DPI_UNAWARE: mode = "DPI Unaware"; break;
            case PROCESS_SYSTEM_DPI_AWARE: mode = "System DPI Aware"; break;
            case PROCESS_PER_MONITOR_DPI_AWARE: mode = "Per-Monitor DPI Aware"; break;
            default: mode = "Unknown";
        }
        std::cout << "Current DPI Awareness Mode: " << mode << std::endl;
	*/
	inline PROCESS_DPI_AWARENESS get_process_dpi_awareness()
	{
		PROCESS_DPI_AWARENESS awareness;
		HANDLE hProcess = ::GetCurrentProcess();
		HRESULT hr = ::GetProcessDpiAwareness(hProcess, &awareness);
		utils_assert(SUCCEEDED(hr));
		return awareness;
	}
}