#pragma once

#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>
#include <utils/windows/process/memory/小工具.hpp>
#include <utils/binary/data/text/提取.hpp>

namespace utils::binary::data::text::提取::小工具
{
	//filename_input: 'utils::windows::memory::小工具::转储所有内存()' 生成的转储结果
	//filename_output: 每行一个文本(utf8编码)
	inline void 提取所有文本(
		const std::filesystem::path & filename_input,
		const std::filesystem::path & filename_output)
	{
		auto blocks = utils::windows::process::memory::小工具::读取转储结果(filename_input);

		utils::FileWriter output{filename_output};
		for(const auto & block : blocks)
		{
			auto GB2312_texts = utils::binary::data::text::提取::GB2312(block.data);
			for (const auto & one : GB2312_texts)
			{
				auto u16 = utils::text::encoding::code_page::to_unicode(one, utils::text::encoding::code_page::CodePage::cp936);
				auto u8 = utils::text::encoding::unicode::utf16_to_utf8(u16);
				output.text.write(utils::text::format::format(u8"%s\n", u8.c_str()));
			}

			auto u16_texts = utils::binary::data::text::提取::u16(block.data);
			for (const auto & one : u16_texts)
			{
				auto u8 = utils::text::encoding::unicode::utf16_to_utf8(one);
				output.text.write(utils::text::format::format(u8"%s\n", u8.c_str()));
			}
		
			auto u8_texts = utils::binary::data::text::提取::u8(block.data);
			for (const auto & one : u8_texts)
			{
				output.text.write(utils::text::format::format(u8"%s\n", one.c_str()));
			}
		}
	}
	//从文本文件中,提取url网址
	//filename_input: 每行一个文本(utf8编码)
	//filename_output: 每行一个url
	inline void 提取http(
		const std::filesystem::path & filename_input,
		const std::filesystem::path & filename_output)
	{
		utils::FileReader input{filename_input};
		std::vector<std::u8string> data_0;
		input.text.read_all_lines(data_0);
	
		std::vector<std::u8string> data_04;
		for(const auto & one : data_0)
		{
			const utils::text::string s = one;
			if(
				s.is_exist(u8"http") &&
				(
					s.is_exist(u8"https://") ||
					s.is_exist(u8"http://") ||
					s.is_exist(u8"https:\\\\") ||
					s.is_exist(u8"http:\\\\")
				))
			{
				auto pos = s.m_text.find(u8"http");
				utils_assert(pos != s.m_text.npos);	//简单检查
				while(1)
				{
					auto url = s.m_text.substr(pos);
					data_04.push_back(url);
				
					pos = s.m_text.find(u8"http", pos+1);
					if(pos == s.m_text.npos)	break;
				}
			}
		}
		std::set<std::u8string> data_05;
		for(const auto & one : data_04)
		{
			std::u8string_view s = one;
		
			{
				auto pos = s.find_first_of(u8R"("<>^`{|} )");
				if (pos != s.npos)
					s = s.substr(0, pos);
			}

			data_05.insert((std::u8string)s);
		}

		struct s_http
		{
			std::u8string line;
			std::u8string protocol;
			std::u8string host;
			std::u8string sub_host;
			std::u8string main_host;
			std::u8string page;
		};
		std::vector<s_http> data_1;
		for(const auto & one : data_05)
		{
			//utils::text::string s = one;
			std::u8string_view _s = one;
		
			s_http new_one;
			new_one.line = _s;

			if(_s.starts_with(u8"http://") || _s.starts_with(u8"http:\\\\"))
			{
				new_one.protocol = u8"http";
				_s.remove_prefix(7);
			}
			else if(_s.starts_with(u8"https://") || _s.starts_with(u8"https:\\\\"))
			{
				new_one.protocol = u8"https";
				_s.remove_prefix(8);
			}
			else
			{
				continue;
			}

			auto pos = _s.find(u8"/");
			if(pos == _s.npos) pos = _s.find(u8"\\");

			if(pos == _s.npos)
			{
				new_one.host = _s;
			}
			else
			{
				new_one.host = _s.substr(0, pos);
				_s.remove_prefix(pos+1);
				new_one.page = _s;

				{
					auto _ss = std::u8string_view(new_one.host);
					auto pos = _ss.rfind(u8".");
					if(pos != _ss.npos) pos = _ss.rfind(u8".", pos-1);
					if(pos != _ss.npos)
					{
						new_one.main_host = _ss.substr(pos+1);
						new_one.sub_host = _ss.substr(0, pos);
					}
					else
						new_one.main_host = _ss;
				}
			}
			data_1.push_back(new_one);
		}

		std::map<std::u8string, std::vector<s_http>> data_2;
		for(const auto & one : data_1)
		{
			data_2[one.main_host].push_back(one);
		}

		for(auto & one : data_2)
		{
			std::sort(one.second.begin(), one.second.end(), [](const auto & a, const auto & b)
			{
				//if(a.main_host != b.main_host) return a.main_host < b.main_host;
				if(a.host != b.host) return a.host < b.host;
				if(a.page != b.page) return a.page < b.page;
				return a.protocol < b.protocol;
			});
		}

		utils::FileWriter output{filename_output};

		for(const auto & group : data_2)
		{
			output.text.write(utils::text::format::format(u8"%s\n", group.first.c_str()));
			for(const auto & one : group.second)
			{
				output.text.write(u8"    ");
				//output.text.write(utils::text::format::old::format("%s ", one.protocol == "http" ? "h" : "H"));
				//if(one.sub_host.empty())
				//	output.text.write(utils::text::format::old::format("* "));
				//else
				//	output.text.write(utils::text::format::old::format("%s.* ", one.sub_host.c_str()));
				//output.text.write(utils::text::format::old::format("%s\n", one.page.c_str()));
				output.text.write_one_line(one.line);
			}
		}
	}
	//整理文本文件, 进行简单排序和去重
	//filename_input: 每行一个文本(utf8编码)
	//filename_output: 每行一个文本
	inline void 整理文本(
		const std::filesystem::path & filename_input,
		const std::filesystem::path & filename_output)
	{
		utils::FileReader input{filename_input};

		std::vector<std::u8string> data_0;
		input.text.read_all_lines(data_0);

		std::map<std::u8string, uint64_t> data_1;
		for(const auto & l : data_0)
		{
			data_1[l]++;
		}

		struct s_line
		{
			std::u8string s;
			uint64_t count;
			double 整齐度;
		};
		std::vector<s_line> data_2;
		for(const auto & l : data_1)
		{
			data_2.push_back({l.first, l.second, utils::binary::data::text::评估::评估(l.first)});
		}

		//抽出一些
		std::vector<s_line> dll;
		std::vector<s_line> data_3;
		for(const auto & one : data_2)
		{
			utils::text::string s = one.s;
		
			if(s.is_exist(u8".dll"))
			{
				dll.push_back(one);
			}
			else
			{
				data_3.push_back(one);
			}
		}

		auto sort = [](std::vector<s_line> & lines)
		{
			std::sort(lines.begin(), lines.end(), [](const auto & a, const auto & b)
			{
				if(a.整齐度 != b.整齐度) return a.整齐度 > b.整齐度;
				if(a.count != b.count) return a.count < b.count;
				return a.s.length() > b.s.length();
			});
		};

		sort(dll);
		sort(data_3);

		utils::FileWriter output{filename_output};
		auto show = [&](const s_line & one)
		{
			output.text.write(utils::text::string().format(u8"%lld %.2lf ", one.count, one.整齐度).m_text);
			output.text.write_one_line(one.s);
		};
		for(const auto & one : dll)
		{
			show(one);
		}
		for(const auto & one : data_3)
		{
			show(one);
		}
	}
}