#pragma once

#include <windows.h>
#include <optional>
#include <utils/assert.hpp>
#include <utils/integer_type.hpp>
#include <utils/windows/handle/ManagedHandle.hpp>

namespace utils::windows::process::token
{
	//打开令牌
	inline auto try_open_token(HANDLE hProcess, DWORD DesiredAccess)
		-> std::optional<utils::windows::handle::Handle>
	{
		HANDLE hToken{};
		if (!OpenProcessToken(hProcess, DesiredAccess, &hToken))
		{
			//简单检查
			utils_assert(ERROR_ACCESS_DENIED == GetLastError());

			return {};
		}

		return {{hToken}};
	}
	//特化
	inline auto open_token(HANDLE hProcess, DWORD DesiredAccess)
		-> utils::windows::handle::Handle
	{
		//特化
		auto hToken = try_open_token(hProcess, DesiredAccess);

		//简单检查
		utils_assert(hToken);

		//给出
		return std::move(*hToken);
	}

	//令牌信息
	//note: 用于配合get_token_information()
	template<typename T>
	struct s_token_information
	{
		std::vector<u1> value_buffer;
		const T* value{};
	};
	//获取令牌信息
	template<typename T>
	inline auto get_token_information(HANDLE hToken, TOKEN_INFORMATION_CLASS TokenInformationClass)
		-> s_token_information<T>
	{
		//获取大小
		DWORD size{};
		utils_assert(!GetTokenInformation(hToken, TokenInformationClass, nullptr, 0, &size));
		utils_assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
		utils_assert(size);

		//建立缓冲区
		s_token_information<T> token_information;
		token_information.value_buffer.resize(size);
		token_information.value = (const T*)&token_information.value_buffer[0];
		
		//获取信息
		DWORD _size{};
		utils_assert(GetTokenInformation(hToken, TokenInformationClass,
			&token_information.value_buffer[0], size, &_size));
		utils_assert(_size == size);

		//给出
		return token_information;
	}
	inline bool is_valid_sid(PSID sid)
	{
		if(!sid) return false;
		return IsValidSid(sid);
	}
	inline u1 get_rid_count(PSID sid)
	{
		//简单检查
		utils_assert(is_valid_sid(sid));

		return *GetSidSubAuthorityCount(sid);
	}
	inline u4 get_rid(PSID sid, DWORD rid_index)
	{
		//简单检查
		utils_assert(rid_index < get_rid_count(sid));

		return *GetSidSubAuthority(sid, rid_index);
	}
}