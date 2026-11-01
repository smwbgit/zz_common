#include <utils/windows/window/s_hwnd.hpp>
#include <utils/windows/tip_window.hpp>
#include <utils/algorithm/filter.hpp>
#include <utils/windows/hook/message_hook.hpp>
#include <utils/windows/SharedMemory.hpp>

void work()
{
	auto hwnd = []()
	{
		//return utils::windows::window::s_hwnd::s_hwnd{(HWND)0x02901C0};
		auto all = utils::windows::window::s_hwnd::get_all_windows();
		utils::algorithm::filter(all, [](const auto & one)
		{
			//return u8"¼ÆËãÆ÷" == one.get_title();
			return u8"demo_target_zz" == one.get_title();
		});
		utils_assert(all.size() == 1);
		return all[0];
	}();
	utils::windows::SharedMemory<DWORD> parent_pid{"zz.240526.02"};
	*parent_pid.operator->() = GetCurrentProcessId();
	Sleep(1000);
	utils::windows::hook::message_hook::install(WH_CBT, R"(E:\zz\src\delete_me\240526\zz_empty2\x64\Debug\Project1.dll)", "CBTProc", hwnd.m_hwnd);

	while (1) Sleep(1);
}
int main()
{
	work();
	return 0;
}