#pragma once

#include <utils/real_path.hpp>
#include <utils/windows/process/create_process.hpp>

namespace utils::product::ffmpeg::视频瘦身
{
	//给一个视频文件，进行瘦身
	//主程序: ffmpeg.exe文件的完整路径
	inline void single_one(
		const std::u8string & 主程序,
		const std::u8string & 输入文件,
		const std::u8string & 输出文件)
	{
		//简单检查
		utils_assert(utils::path::real::get_information::single::is_exist::is_file_exist(主程序));
		utils_assert(utils::path::real::get_information::single::is_exist::is_file_exist(输入文件));
		utils_assert(主程序.ends_with(u8"ffmpeg.exe"));
		utils_assert(输入文件.ends_with(u8".mp4"));
		utils_assert(输出文件.ends_with(u8".mp4"));

		//准备目录
		utils::real_path::create_its_directory_if_not_exist(输出文件);

		//准备数据
		const std::u8string 参数1 = u8R"(-threads 2)";
		//note: 加上，给h265编码器的参数“-x265-params "frame-threads=2:pools=2:lookahead-slices=0:wpp=0"”后，cpu的占用，能从100%到比较小
		const std::u8string 参数2 = u8R"(-fpsmax 30 -c:v libx265 -x265-params "frame-threads=2:pools=2:lookahead-slices=0:wpp=0" -crf 26 -preset medium -vf "scale=-2:720,format=yuv420p" -c:a aac -b:a 128k -ac 2 -tag:v hvc1 -threads 2)";

		//拼出命令
		const std::u8string 完整命令 = u8R"(")" + 主程序 + u8R"(" )" + 参数1 + u8R"( -i ")" + 输入文件 + u8R"(" )" + 参数2 + u8R"( ")" + 输出文件 + u8R"(")";

		//执行命令
		{
			//设置参数
			utils::windows::process::create_process::ProcessCreator process_creator;
			process_creator.m_param.necessary.cmd = 完整命令;
			process_creator.m_param.necessary.is_wait_terminate = true;
			//process_creator.m_param.optional.affinity_mask = 0x3;

			//执行命令
			process_creator.create_process();

			//简单检查
			utils_assert(process_creator.m_result.get_exit_code() == 0);
		}
	}
}