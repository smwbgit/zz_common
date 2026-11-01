// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "framework.h"
#include <thread>
#include <utils/windows/tip_window.hpp>
#include <utils/windows/SharedMemory.hpp>

extern "C" __declspec(dllexport) LRESULT CALLBACK CBTProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	return CallNextHookEx(0, nCode, wParam, lParam);
}
void work(HMODULE hModule)
{
	utils::windows::tip_window::show(u"启动");

	//do sth. here

	utils::windows::tip_window::show(u"完成");

	Sleep(5000); FreeLibraryAndExitThread(hModule, 0);
}
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		std::thread{ [=]()
		{
			utils::windows::SharedMemory<DWORD> parent_pid{"zz.240526.02"};
			DWORD pid = *parent_pid.operator->();
			if(pid == GetCurrentProcessId())
			{
				//MessageBoxA(0,"exit",0,0);
				return;
			}

			MessageBoxA(0,"work",0,0);
			work(hModule);
		} }.detach();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

