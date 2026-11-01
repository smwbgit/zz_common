#pragma once

#include <vector>
#include <utils/assert.hpp>
#include <utils/computer_basic.hpp>
#include <utils/libs/zlib/zlib-1.3/all.hpp>

namespace utils::data_encoding::zlib
{
	//解压缩
	void decode(
		_Out_ std::vector<uint8_t> & to,
		_In_ const std::vector<uint8_t> & from)
	{
		to.clear();

		//无数据直接返回
		if (from.empty())
			return;

		z_stream strm{};
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		utils_assert(Z_OK == inflateInit(&strm));

		utils::computer_basic_function::store_this_integer(strm.avail_in, from.size());
		strm.next_in = (uint8_t*)from.data();
			
		const uint64_t CHUNK = 16384;
		unsigned char out[CHUNK]{};
		int ret{};
		while(1)
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			utils_assert(ret != Z_STREAM_ERROR);
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				utils_throw();
			}
			unsigned have = CHUNK - strm.avail_out;
			to.insert(to.end(), out, out + have);

			if (strm.avail_out)	//直到缓冲区不是全部用完时，全部解压完成
				break;
		}

		utils_assert(ret == Z_STREAM_END);

		(void)inflateEnd(&strm);
	}
}