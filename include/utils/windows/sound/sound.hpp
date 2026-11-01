#pragma once

#include <string>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "utils/assert.hpp"

//声音
namespace utils::windows::api::sound
{
	//播放指定文件
	//note: 播放完毕后才会返回
	//note: 支持格式: mp3, etc..
	inline void play(_In_ const std::string & filename)
	{
		std::string cmd = std::string() + "open \"" + filename + "\" alias sound_haha wait";
		{
			auto re = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
			if (re)
			{
				char text[1000]{};
				auto re2 = mciGetErrorStringA(re, text, 1000);
				re2 = re2;
				//text=="初始化MCI时发生问题" 参见https://www.cnblogs.com/FWFC/p/8868478.html
			}
			utils_assert(0 == re);
//			utils_assert(0 == mciSendStringA(cmd.c_str(), NULL, 0, NULL));
		}
		utils_assert(0 == mciSendStringA("play sound_haha from 0 wait", NULL, 0, NULL));
		utils_assert(0 == mciSendStringA("close sound_haha wait", NULL, 0, NULL));
	}
}