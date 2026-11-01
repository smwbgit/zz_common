#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <utils/data.hpp>

namespace utils::random::safe_random
{
	//生成安全随机数
	//size: 指定需要的字节数
	inline utils::data make(_In_ uint64_t size)
	{
		if (size == 0)
			return {};

		//todo 变为跨平台支持的

		//简单检查
#ifndef utils_on_windows
#error
#endif

		//准备生成器
		HCRYPTPROV handle{};
		utils_assert(CryptAcquireContextA(&handle, NULL, NULL, PROV_RSA_FULL, 0));
		utils_on_leave_scope(utils_assert(CryptReleaseContext(handle, 0)));	//todo 看看怎么处理析构函数中的异常问题

		//准备数据
		uint32_t _size{};
		utils::basic::store_in(_size, size);

		//生成
		utils::data data;
		data.resize(size);
		CryptGenRandom(handle, _size, data.addr());

		//给出
		return data;
	}
}