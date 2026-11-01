#pragma once

#include <vector>
#include "xz/lzma.h"
#include "utils/CallWhenReturn.hpp"
#include "utils/assert.hpp"
#pragma comment(lib, "xz/liblzma.a")

//xz压缩
namespace utils::data_encoding::xz
{
	//todo: 压缩: 参见 https://tukaani.org/xz/xz-5.2.5-windows.7z 里的examples的01_compress_easy.c

	//解压缩
	void decode(
		_Out_ std::vector<uint8_t> & to,
		_In_ const std::vector<uint8_t> & from)
	{
		to.clear();

		//无数据直接返回
		if (from.empty())
			return;

		//创建环境
		lzma_stream stream = LZMA_STREAM_INIT;
		utils::CallWhenReturn clean{[&]()
		{
			//释放资源
			lzma_end(&stream);
		}};

		//初始化解码器
		{
			lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED);
			utils_assert(ret == LZMA_OK);
		}

		//准备缓冲区
		std::vector<uint8_t> buffer;
		buffer.resize(5*1024);

		//准备数据
		stream.next_in = &from[0];
		stream.avail_in = from.size();
		stream.next_out = &buffer[0];
		stream.avail_out = buffer.size();

		while (1)
		{
			//解压
			lzma_ret ret = lzma_code(&stream, LZMA_FINISH);

			//简单检查
			utils_assert(ret == LZMA_STREAM_END || ret == LZMA_OK);

			//保存
			{
				size_t write_size = buffer.size() - stream.avail_out;
				to.insert(to.end(), buffer.begin(), buffer.begin()+write_size);
			}

			//更新状态
			stream.next_out = &buffer[0];
			stream.avail_out = buffer.size();

			//是否全部解压完
			if (ret == LZMA_STREAM_END)
				break;
		}
	}
}
