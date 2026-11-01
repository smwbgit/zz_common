#pragma once

#include <utils/thread/AtomicObject.hpp>
#include <utils/FileWriter.hpp>
#include <utils/u8string.hpp>
#include <utils/text/time/log.hpp>

namespace utils::log
{
	class DebugLogger
	{
	private:
		utils::thread::AtomicObject<utils::FileWriter> m_output;
	public:
		DebugLogger(const std::u8string & filename)
			: m_output{utils::FileWriter::Mode::append, filename}
		{
		}
		void log(const std::u8string & text)
		{
			const auto text_2 = replace(text);
			write_file(text_2);
		}
	private:
		std::u8string replace(const std::u8string & text)
		{
			auto _text = text;
			const auto text_time = utils::text::time::log::get_text();
			utils::u8string(_text).replace_all(u8"<time>", text_time);
			return _text;
		}
		void write_file(const std::u8string & text)
		{
			m_output.lock()->text.write(text);
			m_output.lock()->flush();
		}
	};
}