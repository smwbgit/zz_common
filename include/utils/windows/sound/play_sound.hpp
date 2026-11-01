#pragma once

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <utils/u8string.hpp>
#include <utils/text/encoding.hpp>

//使用 PlaySound 播放 wav 文件
namespace utils::windows::sound::play_sound
{
	namespace inner
	{
		//开始播放
		inline void play(_In_ const std::u8string & filename, DWORD flags)
		{
			//简单检查
			utils_assert(utils::u8string(filename).ends_with(u8".wav"));

			//准备数据
			const auto filename_u16 = utils::text::encoding::utf8_to_utf16(filename);

			//开始播放
			utils_assert(PlaySoundW((LPCWSTR)filename_u16.c_str(), NULL, flags));
		}
	}

	//异步播放 
	inline void play_async(_In_ const std::u8string & filename)
	{
		//转发
		return inner::play(filename, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}
	//同步播放
	inline void play_sync(_In_ const std::u8string & filename)
	{
		//转发
		return inner::play(filename, SND_FILENAME | SND_SYNC | SND_NODEFAULT);
	}
	//循环播放
	inline void play_loop(_In_ const std::u8string & filename)
	{
		//转发
		return inner::play(filename, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT);
	}
}