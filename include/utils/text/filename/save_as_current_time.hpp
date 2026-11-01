#pragma once

#include <filesystem>
#include <utils/text/time/filename.hpp>
#include <utils/random.hpp>
#include <utils/text/hex.hpp>

//拼出一个完整的文件路径。其中，文件名，由当前时间组成
namespace utils::text::filename::save_as_current_time
{
	//postfix: 扩展名
	inline std::u8string make_by_dirname(const std::u8string & dirname, const std::u8string & postfix = u8".bin")
	{
		//获取基本的时间文本
		const auto text_time = utils::text::time::filename::get_text();

		//获取8字节随机值
		const auto random_data = utils::random::make_random(8);

		//把它变为文本形式
		const auto text_random = utils::text::hex::to_text(random_data, false);

		//然后把各段，简单拼出，即可给出了

		//拼出文件名
		const auto filename = text_time + u8"_" + text_random + postfix;

		//拼出完整路径
		const auto _filename = std::filesystem::path(dirname) / filename;

		//给出
		return _filename.u8string();
	}
}
