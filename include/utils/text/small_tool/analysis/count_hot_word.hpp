#pragma once

#include <utils/text/format.hpp>
#include <utils/text/split.hpp>
#include <utils/vector.hpp>

namespace utils::text::small_tool::analysis
{
	inline std::u8string count_hot_word(const std::u8string& text)
	{
		std::u8string text_out;
		{
			std::vector<std::u8string> words;
			{
				utils::text::split::RealImpl<> split{ text };
				split.set_separator_char([](char8_t c)
				{
					if (c & 0x80) return false;
					if (isalnum(c)) return false;
					if (c == u8'_') return false;
					return true;
				});
				split.m_is_keep_empty = false;
				split.set_result_array(words);
				split.execute();
			}

			const auto words_2 = utils::vector(words).count_hot_value();

			for(const auto & entry : words_2)
			{
				text_out += entry.value;
				text_out += u8" ";
				text_out += utils::text::format::format(u8"%lld", entry.count);
				text_out += u8"\n";
			}
		}
		return text_out;
	}
}