#pragma once

#include <utils/assert.hpp>
#include <utils/windows/handle/ManagedHandle.hpp>
#include <utils/windows/process/terminate/exit_code.hpp>
#include <utils/text/encoding.hpp>
#include <utils/data.hpp>

//创建进程
namespace utils::windows::process::create_process
{
	class ProcessCreator
	{
	public:
		//参数
		struct s_param
		{
			struct s_necessary
			{
				std::u8string cmd;	//完整命令
				std::optional<bool> is_wait_terminate;	//是否等待结束
			} necessary;

			struct s_optional
			{
				//std::optional<DWORD> affinity_mask;	//cpu亲和设置
			} optional;
		} m_param;

		//结果
		struct s_result
		{
			utils::windows::handle::Handle process_handle;	//进程句柄
			std::optional<DWORD> exit_code;	//退出码

			//便利函数
		public:
			//获取退出码
			DWORD get_exit_code() const
			{
				//简单检查
				utils_assert(exit_code.has_value());

				//给出
				return exit_code.value();
			}
		} m_result;

	public:
		//创建进程
		void create_process()
		{
			//简单检查
			utils_assert(m_param.necessary.cmd.empty() == false);
			utils_assert(m_param.necessary.is_wait_terminate.has_value());

			//准备数据
			const auto & cmd = m_param.necessary.cmd;
			const auto is_wait_terminate = m_param.necessary.is_wait_terminate.value();
			//const auto & affinity_mask = m_param.optional.affinity_mask;

			//准备数据
			STARTUPINFOW si{};
			si.cb = sizeof(si);

			//准备数据
			auto cmd_u16 = utils::text::encoding::utf8_to_utf16(cmd);

			//创建
			PROCESS_INFORMATION pi{};
			utils_assert(CreateProcessW(
				NULL,
				(wchar_t*)cmd_u16.c_str(),
				NULL, NULL,
				FALSE,
				CREATE_DEFAULT_ERROR_MODE | DETACHED_PROCESS,
				NULL, NULL,
				&si, &pi));

			//关闭无关句柄
			utils_assert(CloseHandle(pi.hThread));

			//保存数据
			m_result.process_handle.set(pi.hProcess);

			//准备数据
			auto & process_handle = m_result.process_handle;

			//等待结束
			if(is_wait_terminate)
			{
				//等待进程结束
				utils::windows::process::terminate::wait_terminate(process_handle);

				//获取退出码
				const auto exit_code = utils::windows::process::terminate::exit_code::
					get_exit_code_of_terminated_process(process_handle);

				//保存数据
				m_result.exit_code = exit_code;
			}
		}
	};

#pragma region 便利函数
	//创建进程
	inline utils::windows::handle::Handle start_process(_In_ const std::u8string & cmd)
	{
		//设置参数
		ProcessCreator process_creator;
		process_creator.m_param.necessary.cmd = cmd;
		process_creator.m_param.necessary.is_wait_terminate = false;

		//创建
		process_creator.create_process();

		//给出
		return std::move(process_creator.m_result.process_handle);
	}
	//特化：启动进程，并等待结束
	//return: 退出码
	inline DWORD start_process_and_wait_exit_code(_In_ const std::u8string & cmd)
	{
		//设置参数
		ProcessCreator process_creator;
		process_creator.m_param.necessary.cmd = cmd;
		process_creator.m_param.necessary.is_wait_terminate = true;

		//创建
		process_creator.create_process();

		//给出
		return process_creator.m_result.get_exit_code();
	}
#pragma endregion
}