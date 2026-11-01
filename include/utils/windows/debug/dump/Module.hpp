#pragma once

#include <windows.h>
#include <utils/windows/process.hpp>
#include <utils/windows/pe.hpp>
#include <utils/inner/windows/dump.hpp>
#include <utils/math/cover.hpp>
#include <utils/DataWriter.hpp>

namespace utils::windows::dump
{
	//进程空间中的一个dll或exe
	class Module : public utils::inner::windows::dump::Base<Module>
	{
	private:
		utils::data m_data;
	public:
		//从一个模块的dump修复为dll或exe文件
		//note: 修复内容: 紧缩各个段，恢复导入表，恢复重定位，段的数量
		//note: 还有全局数据没有和原始exe一致
		//is_keep_all: 是否保留所有段。保留的话，段会放在最后
		//old_ImageBase: 原始ImageBase。不传则不做重定位修正
		void restore(const std::string & filename, bool is_keep_all = false, std::optional<uint64_t> old_ImageBase = {})
		{
			utils::data data_1 = m_data;

			//收缩各个段
			utils::data data_2 = data_1;
			{
				utils::windows::pe::PeViewer pe_viewer{data_1.addr(), false};

				uint64_t exe_size{};
				std::vector<std::pair<uint64_t, uint64_t>> used_spaces_mem;	//用于校验是否空间有重叠
				std::vector<std::pair<uint64_t, uint64_t>> used_spaces_raw;	//用于校验是否空间有重叠
				auto check_and_record = [](std::vector<std::pair<uint64_t, uint64_t>> & used_spaces, uint64_t addr, uint64_t size)
				{
					if (size > 0)
					{
						//检查是否重叠
						for (const auto & one : used_spaces)
							utils_assert(!utils::math::cover::is_overlap(std::get<0>(one), std::get<1>(one), addr, size));

						used_spaces.push_back({addr, size});
					}
				};
				uint64_t section_n{};
				for (const auto & one : pe_viewer.get_section_table_iterator())
				{
					//简单检查
					check_and_record(used_spaces_mem, one.VirtualAddress, one.Misc.VirtualSize);
					check_and_record(used_spaces_raw, one.PointerToRawData, one.SizeOfRawData);

					//更新数据
					section_n++;

					if (one.SizeOfRawData > 0)
					{
						//准备数据
						utils::data data;
						data.resize(one.SizeOfRawData);
						uint64_t size = (std::min)(one.SizeOfRawData, one.Misc.VirtualSize);
						memmove(data.addr(), data_1.addr() + one.VirtualAddress, size);

						//简单检查
						utils_assert((uint64_t)(one.PointerToRawData + one.SizeOfRawData) <= data_2.size());

						//更改
						memmove(data_2.addr() + one.PointerToRawData, data.addr(), data.size());

						//更新数据
						exe_size = (std::max)(exe_size, (uint64_t)(one.PointerToRawData + one.SizeOfRawData));
					}
				}

				//简单检查
				utils_assert(exe_size > 0);
				utils_assert(exe_size <= data_2.size());

				//收缩
				data_2.resize(exe_size);

				//修复段数量
				{
					utils::windows::pe::PeViewer pe_viewer{data_2.addr(), true};
					auto & NumberOfSections = pe_viewer.get_nt_headers().FileHeader.NumberOfSections;
					if (section_n != NumberOfSections)
					{
						//简单检查
						utils_assert(section_n > NumberOfSections);

						//修复
						utils::basic::store_in(NumberOfSections, section_n);
					}
				}
			}

			if (is_keep_all)
			{
				utils::data data_3;
				utils::windows::pe::PeViewer pe_viewer{data_2.addr(), true};

				for (auto & one : pe_viewer.get_section_table_iterator())
				{
					if (one.SizeOfRawData == 0)
					{
						//简单检查
						utils_assert(one.Misc.VirtualSize > 0);

						//准备数据
						utils::data data;
						data.resize(utils::math::round::bigger(one.Misc.VirtualSize, 0x200));
						memmove(data.addr(), data_1.addr() + one.VirtualAddress, one.Misc.VirtualSize);

						//更新段表
						one.PointerToRawData = data_2.size() + data_3.size();
						one.SizeOfRawData = data.size();

						//添加
						data_3.append(data);
					}
				}
				data_2.append(data_3);
			}

			utils::windows::pe::PeViewer pe_viewer_2{data_2.addr(), true};

			//处理导入表
			for (const auto & one : pe_viewer_2.get_import_table_iterator())
			{
				auto from = (IMAGE_THUNK_DATA*)pe_viewer_2.get_by_rva(one.OriginalFirstThunk);
				auto to = (IMAGE_THUNK_DATA*)pe_viewer_2.get_by_rva(one.FirstThunk);

				{
					printf("FirstThunk: 0x%x\n", one.FirstThunk);
				}

				for (uint64_t i = 0; ; i++)
				{
					if (utils::basic::is_zero(from[i])) break;

					to[i] = from[i];
				}
			}

			//处理重定位
			if (old_ImageBase.has_value())
			{
				const int64_t reloc_offset = (int64_t)old_ImageBase.value() - (int64_t)pe_viewer_2.get_nt_headers().OptionalHeader.ImageBase;
				for (const auto & one : pe_viewer_2.get_reloc_table_detail())
				{
					for (const auto & loc : std::get<1>(one))
					{
						//简单检查
						utils_assert((loc & 0xf000) == 0xa000);

						*(uint64_t*)pe_viewer_2.get_by_rva(std::get<0>(one).VirtualAddress + (loc & 0xfff)) += reloc_offset;
					}
				}
				pe_viewer_2.get_nt_headers().OptionalHeader.ImageBase += reloc_offset;
			}

			utils::FileWriter output{filename};
			output.write(data_2);
		}
		//通过对各个段的8字节比对，重新建立导入表
		static void restore_import_table(const std::string & input, const utils::windows::dump::ExportTables & export_tables, const std::string & output)
		{

		}

	private:
		friend class utils::inner::windows::dump::Base<Module>;

		Module()
		{
		}
		//收集信息
		//dll_name: 不带路径 e.g. "user32.dll"
		static utils::data gather(const std::string & dll_name)
		{
			auto dlls = utils::windows::process::get_dlls();

			utils::CountChecker count_checker{1};
			utils::data data;
			for (const auto & dll : dlls.m_dlls)
			{
				if (utils::to_lib_type(dll.base_name).string_view().iequals(dll_name))
				{
					//简单检查
					count_checker.count();
					utils_assert((dll.base & 0xfff) == 0);
					utils_assert(dll.size >= 0x1000);
					utils_assert((dll.size & 0xfff) == 0);

					utils::DataWriter output;
					for (auto p = dll.base; p < dll.base + dll.size; p += 0x1000)
					{
						if (utils::windows::memory::is_readable(p))
							output.write((uint8_t*)p, 0x1000);
						else
							output.write_some((uint8_t)0, 0x1000);
					}
					output.get_data_and_clear(data);
				}
			}
			return data;
		}
		//执行分析
		static Module analysis(utils::data && data)
		{
			Module one;
			one.m_data = std::move(data);
			return one;
		}
	};
}