#pragma once

#include <windows.h>
#include <optional>
#include <utils/windows/process/open_process.hpp>
#include <utils/windows/process/token.hpp>

namespace utils::windows::process::integrity_level
{
	//代表一种完整性级别
	class IntegrityLevel
	{
	public:
		enum class Level
		{
			untrusted,
			low,
			medium,
			high,
			system,
			protected_process,
		};
	private:
		static Level get_level(DWORD rid)
		{
			if(rid < SECURITY_MANDATORY_LOW_RID)
				return  Level::untrusted;
			if(rid < SECURITY_MANDATORY_MEDIUM_RID)
				return  Level::low;
			if(rid < SECURITY_MANDATORY_HIGH_RID)
				return  Level::medium;
			if(rid < SECURITY_MANDATORY_SYSTEM_RID)
				return  Level::high;
			if(rid < SECURITY_MANDATORY_PROTECTED_PROCESS_RID)
				return  Level::system;
			return  Level::protected_process;
		}
		static DWORD get_basic_rid(Level level)
		{
			switch(level) {
			case Level::untrusted:	return SECURITY_MANDATORY_UNTRUSTED_RID;
			case Level::low:		return SECURITY_MANDATORY_LOW_RID;
			case Level::medium:		return SECURITY_MANDATORY_MEDIUM_RID;
			case Level::high:		return SECURITY_MANDATORY_HIGH_RID;
			case Level::system:		return SECURITY_MANDATORY_SYSTEM_RID;
			case Level::protected_process:	return SECURITY_MANDATORY_PROTECTED_PROCESS_RID;
			}
			utils_throw();
		}
	private:
		std::optional<Level> m_level;
		std::optional<DWORD> m_rid;
	public:
		IntegrityLevel()
		{
		}
		explicit IntegrityLevel(DWORD rid)
			: m_rid{rid}
		{
			m_level = get_level(rid);
		}
		IntegrityLevel(Level level)
			: m_level{level}
		{
			m_rid = get_basic_rid(level);
		}
		std::u8string get_text() const
		{
			//简单检查
			utils_assert(m_level.has_value());

			switch(m_level.value()) {
			case Level::untrusted:	return u8"1.untrusted";
			case Level::low:		return u8"2.low";
			case Level::medium:		return u8"3.medium";
			case Level::high:		return u8"4.high";
			case Level::system:		return u8"5.system";
			case Level::protected_process:	return u8"6.protected_process";
			}
			utils_throw();
		}
		std::weak_ordering operator <=> (const IntegrityLevel & o) const
		{
			//简单检查
			utils_assert(m_level.has_value());
			utils_assert(o.m_level.has_value());

			return m_level.value() <=> o.m_level.value();
		}
	};
	//获得指定进程的完整性级别
	inline std::optional<IntegrityLevel> get_integrity_level(DWORD pid)
	{
		const auto hProcess = utils::windows::process::try_open_process(pid,
			PROCESS_QUERY_LIMITED_INFORMATION);
		if(!hProcess) return {};
		
		const auto hToken = utils::windows::process::token::try_open_token(*hProcess, TOKEN_QUERY);

		//todo: 看看有没有打不开的情况。如果有，直接返回{}
		//简单检查
		utils_assert(hToken);

		const auto information = utils::windows::process::token::
			get_token_information<TOKEN_MANDATORY_LABEL>(*hToken, TokenIntegrityLevel);

		//简单检查
		utils_assert(1 == utils::windows::process::token::
			get_rid_count(information.value->Label.Sid));

		const auto rid = utils::windows::process::token::
			get_rid(information.value->Label.Sid, 0);

		return IntegrityLevel{rid};
	}
	//判断指定进程的完整性，是否高于指定值
	inline bool is_too_high(DWORD pid, IntegrityLevel level = IntegrityLevel::Level::medium)
	{
		const auto il = utils::windows::process::integrity_level::
			get_integrity_level(pid);

		//查询失败
		if(!il.has_value())
			return true;

		//给出判断
		return il.value() > level;
	}
}