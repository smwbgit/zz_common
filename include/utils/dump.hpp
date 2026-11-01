#pragma once

#include <utils/FileWriter.hpp>

namespace utils::dump
{
	inline void dump_text_to_file(const std::u8string & text, const std::u8string & filename)
	{
		utils::FileWriter output{filename};
		output.write(text.data(), text.size());
	}
}