#pragma once

#include <string>
#include <utils/windows/command.hpp>
#include <utils/real_path.hpp>

namespace utils::windows::sound::windows_media_player
{
	//使用Windows Media Player播放声音文件, Windows Media Player被关掉后，才会返回
	inline void play(const std::string & filename)
	{
		auto _filename = utils::real_path::get_absolute(filename);
		utils::windows::command::run(R"("C:\Program Files (x86)\Windows Media Player\wmplayer.exe" /prefetch:6 /open ")" + _filename + "\"", false);
	}
}