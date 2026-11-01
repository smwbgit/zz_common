#pragma once

#include <functional>
#include <tuple>

namespace utils::callback
{
	//能让lambda函数，做为（回调函数，自定义参数）这种形式传入
	//note: 传入的参数会在内部按值传递几次，所以参数是大对象时要考虑重写/升级一下这个类
	//addr_index: 参数列表中，第几个参数是用来传递this指针的
#if 0
	用法举例
		uint64_t count{};
		utils::CallbackAdapter< 1, BOOL, HWND, LPARAM > callback {
			[&count](HWND hwnd, LPARAM lParam) ->BOOL  {
				count++;
				return TRUE; /* 继续枚举 */  }};
		EnumThreadWindows(GetCurrentThreadId(), callback.callback_stdcall(), callback.param());
#endif
	template<uint64_t addr_index, typename return_T, typename ... args_T>
	class CallbackAdapter
	{
	private:
		using temp_a_1 = return_T (*)(args_T...);
		using temp_a_2 = return_T (__stdcall *)(args_T...);
	private:
		std::function<return_T(args_T...)> m_func;
	public:
		CallbackAdapter(std::function<return_T(args_T...)> m_func)
			: m_func{ m_func }
		{
		}
		temp_a_1 callback_cdecl()
		{
			return temp_1;
		}
		temp_a_2 callback_stdcall()
		{
			return temp_2;
		}
		uint64_t param()
		{
			return (uint64_t)this;
		}
	private:
		static return_T temp_1(args_T ... args)
		{
			auto _this = (CallbackAdapter*)std::get<addr_index>(std::tuple<args_T...>(args...));

			return _this->m_func(args ...);
		}
		static return_T __stdcall temp_2(args_T ... args)
		{
			//转发
			return temp_1(args ...);
		}
	};
}