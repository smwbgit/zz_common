#pragma once

#include <windows.h>
#include <utils/data.hpp>
#include <utils/basic.hpp>
#include <utils/iterator.hpp>
#include <utils/map.hpp>
#include <utils/math/round.hpp>
#include <utils/CountChecker.hpp>
#include <utils/DataReader.hpp>
#include <utils/FileReader.hpp>
#include <utils/MarkedLife.hpp>
#include <utils/text/format.hpp>
#include <utils/algorithm.hpp>
//#include <utils/character_encoding.hpp>
#include <utils/algorithm/ranges.hpp>
#include <utils/windows/undocumented.hpp>
#include <utils/binary/code/concepts/VirtualAddressReader.hpp>

//PE文件
namespace utils::windows::pe
{
#if 0
	class PeLooker
	{
	public:
		IMAGE_NT_HEADERS64* m_headers{};
	private:
		uint64_t m_base{};
		bool m_is_in_file{};
	public:
		//is_in_file: 模块存在形式: 文件形式的紧缩状态 或 内存中的展开状态。影响rva如何使用节数据
		PeLooker(void* base, bool is_in_file)
			: m_base{(uint64_t)base}
			, m_is_in_file{is_in_file}
		{
			auto dos = (IMAGE_DOS_HEADER*)m_base;

			//简单检查
			utils_assert(dos->e_magic == IMAGE_DOS_SIGNATURE);

			m_headers = (IMAGE_NT_HEADERS64*)(m_base + dos->e_lfanew);

			//简单检查
			utils_assert(m_headers->Signature == IMAGE_NT_SIGNATURE);
		}
		auto get_section_table() const
		{
			auto sections = (IMAGE_SECTION_HEADER*)((int64_t)m_headers + (int64_t)sizeof(IMAGE_NT_HEADERS64) + ((int64_t)m_headers->OptionalHeader.NumberOfRvaAndSizes - (int64_t)IMAGE_NUMBEROF_DIRECTORY_ENTRIES) * sizeof(IMAGE_DATA_DIRECTORY));

#if 0	//有节的数量大于n的情况
			auto n = m_headers->FileHeader.NumberOfSections;

			//简单检查
			utils_assert(utils::basic::is_zero(sections[n]));
#endif

			return utils::iterator::windows::pe::vector(sections);
		}
		std::vector<IMAGE_IMPORT_DESCRIPTOR> get_import_table()
		{
			auto rva = m_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
			if(rva == 0) return {};
			auto table = (IMAGE_IMPORT_DESCRIPTOR*)get_by_rva(rva);

			std::vector<IMAGE_IMPORT_DESCRIPTOR> _table;
			for(uint64_t i = 0; ;i++)
			{
				if(utils::basic::is_zero(table[i])) break;

				_table.push_back(table[i]);
			}
			return _table;
		}
		auto get_export_table()
		{
			struct s_one_function
			{
				std::string name;

				uint64_t rva{};	//valid when is_forward==false
				bool is_forward{};
				std::string forward_dll;		//valid when is_forward==true
				std::string forward_func_name;	//valid when is_forward==true
			};
			std::vector<s_one_function> exported_functions;

			auto rva = m_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			if(rva == 0) return exported_functions;
			auto _export = (IMAGE_EXPORT_DIRECTORY*)get_by_rva(rva);
			uint64_t _export_size = m_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

			//简单检查
			utils_assert(_export_size > 0);

			auto func_n = _export->NumberOfFunctions;
			auto name_n = _export->NumberOfNames;
			auto funcs = (uint32_t*)get_by_rva(_export->AddressOfFunctions);
			auto names = (uint32_t*)get_by_rva(_export->AddressOfNames);
			auto name_ordinals = (uint16_t*)get_by_rva(_export->AddressOfNameOrdinals);

			for(auto i : utils::iterator_index(func_n))
			{
				s_one_function one_function{};
				one_function.name = [&]()->std::string
				{
					for(auto j : utils::iterator_index(name_n))
					{
						if(name_ordinals[j] == i)
							return (char*)get_by_rva(names[j]);
					}
					return utils::text::format("ordinal 0x%llx", i + _export->Base);
				}();

				if ([&]()
				{
					auto addr = get_by_rva(funcs[i]);
					return addr >= (uint64_t)_export && addr < (uint64_t)_export + _export_size;
				}())
				{
					//函数转发

					utils::string text = (char*)get_by_rva(funcs[i]);
					one_function.is_forward = true;

					auto fragments = text.string_view().split(".");

					//简单检查
					utils_assert(fragments.size() == 2);
					utils_assert(!fragments[0].empty());
					utils_assert(!fragments[1].empty());

					one_function.forward_dll = fragments[0];
					one_function.forward_func_name = fragments[1];
				}
				else
				{
					//通常的, 非转发的

					one_function.is_forward = false;
					one_function.rva = funcs[i];
				}
				exported_functions.push_back(std::move(one_function));
			}
			return exported_functions;
		}
		std::vector<std::tuple<IMAGE_BASE_RELOCATION, std::vector<uint16_t>>> get_reloc_table()
		{
			auto rva = m_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
			if(rva == 0) return {};
			auto table = (IMAGE_BASE_RELOCATION*)get_by_rva(rva);

			std::vector<std::tuple<IMAGE_BASE_RELOCATION, std::vector<uint16_t>>> _table;
			for(auto one = table; !utils::basic::is_zero(*one); one = (IMAGE_BASE_RELOCATION*)((uint64_t)one+one->SizeOfBlock))
			{
				std::vector<uint16_t> locations;
				{
					//简单检查
					utils_assert((one->VirtualAddress&0xfff) == 0);
					utils_assert(one->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION));
					utils_assert((one->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))%2 == 0);

					auto block = (uint16_t*)((uint64_t)one+sizeof(IMAGE_BASE_RELOCATION));

					auto size = (one->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/2;

					for(auto i : utils::iterator_index(size))
					{
						//简单检查
						utils_assert((block[i]&0xf000) == 0 || (block[i]&0xf000) == 0xa000);

						if((block[i]&0xf000) == 0)
						{
							//简单检查
							//utils_assert(block[i] == 0);	//有不是0的
							utils_assert(i == size-1);

							continue;
						}

						locations.push_back(block[i]);
					}
				}
				
				_table.push_back({*one, locations});
			}
			return _table;
		}
		uint64_t get_by_rva(uint64_t rva)
		{
			if(m_is_in_file)
			{
				for(const auto & one : get_section_table())
				{
					if(rva >= one.VirtualAddress && rva < one.VirtualAddress + one.Misc.VirtualSize)
						return m_base + rva - one.VirtualAddress + one.PointerToRawData;
				}
				utils_throw();
			}
			else
				return m_base + rva;
		}
	};
#endif
	
	//PE查看器
	//note: 不会对其中的数据进行重定位
	class PeViewer
	{
	public:
		//一个导出函数
		struct s_export_function
		{
			std::u8string m_name;

			bool m_is_forward{};
			uint64_t m_rva{};					//valid when is_forward==false
			std::u8string m_forward_dll;		//valid when is_forward==true
			std::u8string m_forward_func_name;	//valid when is_forward==true
		};

	private:
		uint64_t m_base{};
		bool m_is_in_file{};
	public:
		//is_in_file: 模块存在形式。true:文件形式 false:加载后的形式。影响rva如何使用段表，来确定数据的位置
		PeViewer(const void* base, bool is_in_file)
		{
			//简单检查
			utils_assert(base);

			m_base = (uint64_t)base;
			m_is_in_file = is_in_file;
		}
		bool is_x64() const
		{
			const auto & dos = *(IMAGE_DOS_HEADER*)m_base;

			//简单检查
			utils_assert(dos.e_magic == IMAGE_DOS_SIGNATURE);

			IMAGE_NT_HEADERS & nt_headers = *(IMAGE_NT_HEADERS*)(m_base + dos.e_lfanew);

			return nt_headers.FileHeader.Machine == (uint16_t)0x8664;
		}
		const IMAGE_NT_HEADERS & get_nt_headers() const
		{
			const auto & dos = *(IMAGE_DOS_HEADER*)m_base;

			//简单检查
			utils_assert(dos.e_magic == IMAGE_DOS_SIGNATURE);

			IMAGE_NT_HEADERS & nt_headers = *(IMAGE_NT_HEADERS*)(m_base + dos.e_lfanew);

			//简单检查
			utils_assert(nt_headers.Signature == IMAGE_NT_SIGNATURE);
			utils_assert(nt_headers.FileHeader.Machine == (uint16_t)0x8664);
			//utils_assert(nt_headers.OptionalHeader.FileAlignment == 0x200 || nt_headers.OptionalHeader.FileAlignment == 0x1000);
			//utils_assert(nt_headers.OptionalHeader.SectionAlignment == 0x1000 || nt_headers.OptionalHeader.SectionAlignment == 0x2000);

			return nt_headers;
		}
	private:
		//获取节表
		const IMAGE_SECTION_HEADER* get_section_table() const
		{
			auto & nt_headers = get_nt_headers();

			//简单检查
			utils_assert(nt_headers.OptionalHeader.NumberOfRvaAndSizes >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES);

			const auto rest_n = (uint64_t)(nt_headers.OptionalHeader.NumberOfRvaAndSizes - IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
			const auto addr = (uint64_t)&nt_headers + sizeof(IMAGE_NT_HEADERS) + rest_n * sizeof(IMAGE_DATA_DIRECTORY);
			return (const IMAGE_SECTION_HEADER*)addr;
		}
	public:
		//获取节表的迭代器
		auto get_section_table_iterator() const
		{
			auto & nt_headers = get_nt_headers();
			auto section_table = get_section_table();

			//简单检查
			utils_assert(nt_headers.FileHeader.NumberOfSections);

			return utils::iterator::array<const IMAGE_SECTION_HEADER>(section_table, nt_headers.FileHeader.NumberOfSections);
		}
		//按名字获取节表
		const IMAGE_SECTION_HEADER & get_section_by_name(const std::u8string & name) const
		{
			auto & section = utils::algorithm::ranges::find_unique(get_section_table_iterator(), [&](const auto & one)
			{
				if (one.Name[7] == 0)
					return (const char8_t*)one.Name == name;
				else
					return std::u8string((const char8_t*)one.Name, 8) == name;
			});

#if 0
			//简单检查
			if(name == u8".pdata")
			{
				utils_assert(
					section.Characteristics == (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ) ||
					section.Characteristics == (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_NOT_PAGED)
				);
				utils_assert(section.Misc.VirtualSize <= section.SizeOfRawData);
				utils_assert(section.Misc.VirtualSize % sizeof(RUNTIME_FUNCTION) == 0);
			}
#endif

			return section;
		}
		//是否存在指定节表
		bool is_exist_section(const std::u8string & name) const
		{
			for(const auto & one : get_section_table_iterator())
			{
				const auto _name = [&]() -> std::u8string
				{
					if (one.Name[7] == 0)
						return (const char8_t*)one.Name;
					else
						return std::u8string((const char8_t*)one.Name, 8);
				}();
				if(name == _name)
					return true;
			}
			return false;
		}
		//模块大小，当在内存中载入时
		uint64_t get_module_size() const
		{
			return get_nt_headers().OptionalHeader.SizeOfImage;
		}
		//用rva获得m_base后的某个位置
		//size: 要读取的大小
		template<typename T>
		const T get_by_rva(uint64_t rva, uint64_t size) const
		{
			//简单检查
			static_assert(std::is_pointer_v<T>);

			if (m_is_in_file)
			{
				for (const auto & one : get_section_table_iterator())
				{
					if (rva >= one.VirtualAddress && rva < one.VirtualAddress + one.Misc.VirtualSize)
					{
						//简单检查
						utils_assert(rva + size <= one.VirtualAddress + one.Misc.VirtualSize);

						return (const T)(m_base + rva - one.VirtualAddress + one.PointerToRawData);
					}
				}
				utils_throw();
			}
			else
				return (const T)(m_base + rva);
		}
		//用rva获得一个以0结尾的字符串 
		//note: 以一个字节0结尾，所以不能处理unicode字符串
		std::u8string get_ending_zero_string_by_rva(uint64_t rva) const
		{
			if (m_is_in_file)
			{
				for (const auto & one : get_section_table_iterator())
				{
					if (rva >= one.VirtualAddress && rva < one.VirtualAddress + one.Misc.VirtualSize)
					{
						//剩余字节
						uint64_t rest_size = one.VirtualAddress + one.Misc.VirtualSize - rva;

						//开始位置
						auto addr = (const char8_t*)(m_base + rva - one.VirtualAddress + one.PointerToRawData);

						//寻找结尾0
						for(uint64_t i = 0; i < rest_size; i++)
						{
							//找到
							if (*(addr+i) == 0)
								return std::u8string(addr, i);
						}

						//没有找到结尾0
						utils_throw();
					}
				}
				utils_throw();
			}
			else
				return (const char8_t*)(m_base + rva);
		}
		//获取目录项
		const IMAGE_DATA_DIRECTORY & get_directory_entry(uint64_t index) const
		{
			auto & nt_headers = get_nt_headers();
			return nt_headers.OptionalHeader.DataDirectory[index];
		}
		//获取RUNTIME_FUNCTION数组
		auto get_runtime_functions_iterator() const
		{
			//获取目录项
			const auto & directory_entry = get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXCEPTION);

			//没有数据，返回空数组
			if (directory_entry.VirtualAddress == 0)
				return utils::iterator::array<RUNTIME_FUNCTION>(nullptr, 0);

			//简单检查
			utils_assert(directory_entry.Size % sizeof(RUNTIME_FUNCTION) == 0);

			//获取数据位置
			const auto runtime_functions = get_by_rva<RUNTIME_FUNCTION*>(
				directory_entry.VirtualAddress, directory_entry.Size);

			//给出
			return utils::iterator::array(runtime_functions,
					directory_entry.Size / sizeof(RUNTIME_FUNCTION));
		}
		//获取一个UNWIND_INFO
		auto get_unwind_info(const RUNTIME_FUNCTION & one) const
		{
			//简单检查
			utils_assert((one.UnwindInfoAddress & 1) == 0);
			utils_assert((one.UnwindInfoAddress & 2) == 0);

			//todo 让size包含exception handler

			using namespace utils::windows::undocumented::unwind;
			auto head = get_by_rva<UNWIND_INFO*>(one.UnwindData, sizeof(UNWIND_INFO) - sizeof(UNWIND_CODE));
			uint64_t size = sizeof(UNWIND_INFO) - sizeof(UNWIND_CODE) + sizeof(UNWIND_CODE)*head->CountOfCodes;
			return get_by_rva<UNWIND_INFO*>(one.UnwindData, size);
		}
		//获取导入表的迭代器
		auto get_import_table_iterator()
		{
			//查找导入表
			const auto & directory_entry = get_directory_entry(IMAGE_DIRECTORY_ENTRY_IMPORT);

			//没有导入表，返回空的迭代器
			if (directory_entry.VirtualAddress == 0)
			{
				return utils::iterator::array<IMAGE_IMPORT_DESCRIPTOR>(nullptr, 0);
			}

			//简单检查
			utils_assert(directory_entry.Size % sizeof(IMAGE_IMPORT_DESCRIPTOR) == 0);

			//获得真实地址
			auto import_table = get_by_rva<IMAGE_IMPORT_DESCRIPTOR*>(
				directory_entry.VirtualAddress, directory_entry.Size);

			return utils::iterator::array(import_table,
				directory_entry.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR));
		}
		//是否存在导出表
		bool is_exist_export_table()
		{
			//获得目录项
			auto export_table = get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXPORT);
			
			return export_table.VirtualAddress != 0;
		}
		//获得导出表
		const IMAGE_EXPORT_DIRECTORY & get_export_table_header()
		{
			//获得目录项
			auto export_table = get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXPORT);
			
			//简单检查
			utils_assert(export_table.VirtualAddress && export_table.Size >= sizeof(IMAGE_EXPORT_DIRECTORY));

			//获得真实地址
			const auto export_table_header = get_by_rva<IMAGE_EXPORT_DIRECTORY*>(
				export_table.VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
			
			return *export_table_header;
		}
		//获得导出表的详细信息
		std::vector<s_export_function> get_export_table_detail()
		{
			//不存在导出表，返回一个空数组
			if (!is_exist_export_table()) return {};

			//获得目录项
			auto directory_entry = get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXPORT);
			
			//获得导出表的头
			const auto header = get_export_table_header();

			//读取各项数据
			auto func_n = header.NumberOfFunctions;
			auto name_n = header.NumberOfNames;
			auto funcs = get_by_rva<uint32_t*>(header.AddressOfFunctions, sizeof(uint32_t)*func_n);
			auto names = get_by_rva<uint32_t*>(header.AddressOfNames, sizeof(uint32_t)*name_n);
			auto ordinals = get_by_rva<uint16_t*>(header.AddressOfNameOrdinals, sizeof(uint16_t)*name_n);

			//遍历每个导出函数
			std::vector<s_export_function> export_functions;
			for (const auto i : utils::iterator_index(func_n))
			{
				s_export_function function{};
				function.m_name = [&]()->std::u8string
				{
					for (const auto j : utils::iterator_index(name_n))
					{
						if (ordinals[j] == i)
							return get_ending_zero_string_by_rva(names[j]);
					}
					return utils::text::format::format(u8"ordinal 0x%llx", i + header.Base);
				}();

				//如果在导出目录中，说明是转发函数
				if (funcs[i] >= directory_entry.VirtualAddress &&
					funcs[i] < directory_entry.VirtualAddress + directory_entry.Size)
				{
					//函数转发

					std::u8string text = get_ending_zero_string_by_rva(funcs[i]);
					function.m_is_forward = true;

					auto fragments = utils::u8string(text).split(u8".");

					//简单检查
					utils_assert(fragments.size() == 2);
					utils_assert(!fragments[0].empty());
					utils_assert(!fragments[1].empty());

					function.m_forward_dll = fragments[0];
					function.m_forward_func_name = fragments[1];
				}
				else
				{
					//通常的, 非转发的

					function.m_is_forward = false;
					function.m_rva = funcs[i];
				}
				export_functions.push_back(std::move(function));
			}

			//给出
			return export_functions;
		}
		//通过入口点获取导出函数名
		//note: 入口点是rva
		std::optional<std::u8string> get_export_function_name_by_entry_point(uint64_t rva)
		{
			const auto funcs = get_export_table_detail();
			for(const auto & one : funcs)
			{
				if (one.m_is_forward) continue;
				if (one.m_rva == rva)
					return one.m_name;
			}
			return {};
		}
	public:
		class VirtualAddressReader
		{
		private:
			PeViewer & m_this;
			uint64_t m_base{};

			//禁止简单复制
		public:
			VirtualAddressReader() = delete;
			VirtualAddressReader(const VirtualAddressReader &) = delete;
			VirtualAddressReader(VirtualAddressReader &&) = delete;
			VirtualAddressReader & operator =(const VirtualAddressReader &) = delete;
			VirtualAddressReader & operator =(VirtualAddressReader &&) = delete;

		public:
			VirtualAddressReader(PeViewer & _this, uint64_t base)
				: m_this{_this}
				, m_base{base}
			{
			}
			void read_random(uint64_t position, uint8_t* buffer, uint64_t size)
			{
				utils_assert(position >= m_base);
				auto addr = m_this.get_by_rva<uint8_t*>(position - m_base, size);
				memmove(buffer, addr, size);
			}
		}; static_assert(utils::binary::code::concepts::VirtualAddressReader<VirtualAddressReader>);

	public:
		auto create_virtual_address_reader(uint64_t base)
		{
			return VirtualAddressReader{*this, base};
		}
#if 0	//todo 待修
		//获取重定位表
		IMAGE_BASE_RELOCATION* get_reloc_table()
		{
			return (IMAGE_BASE_RELOCATION*)get_directory_entry_addr(IMAGE_DIRECTORY_ENTRY_BASERELOC).value_or(nullptr);
		}
		//获取重定位表的详细信息
		std::vector<std::tuple<IMAGE_BASE_RELOCATION, std::vector<uint16_t>>> get_reloc_table_detail()
		{
			auto reloc_table = get_reloc_table();
			if (reloc_table == nullptr) return {};

			std::vector<std::tuple<IMAGE_BASE_RELOCATION, std::vector<uint16_t>>> _table;
			for (auto one = reloc_table; !utils::basic::is_zero(*one); one = (IMAGE_BASE_RELOCATION*)((uint64_t)one + one->SizeOfBlock))
			{
				std::vector<uint16_t> locations;
				{
					//简单检查
					utils_assert((one->VirtualAddress & 0xfff) == 0);
					utils_assert(one->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION));
					utils_assert((one->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) % 2 == 0);

					auto block = (uint16_t*)((uint64_t)one + sizeof(IMAGE_BASE_RELOCATION));

					auto size = (one->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2;

					for (auto i : utils::iterator_index(size))
					{
						//简单检查
						utils_assert((block[i] & 0xf000) == 0 || (block[i] & 0xf000) == 0xa000);

						if ((block[i] & 0xf000) == 0)
						{
							//简单检查
							//utils_assert(block[i] == 0);	//有不是0的
							utils_assert(i == size - 1);

							continue;
						}

						locations.push_back(block[i]);
					}
				}

				_table.push_back({*one, locations});
			}
			return _table;
		}
#endif
	};

#if 0 //待修
	namespace inner
	{
		class PeEditor
		{
		private:
			utils::data m_data;
			bool m_is_in_file{};
		public:
			//is_in_file: 参见PeViewer的参数is_in_file的说明
			PeEditor(const utils::data & data, bool is_in_file)
			{
				m_data = data;
				m_is_in_file = is_in_file;
			}
			utils::data finish()
			{
				return m_data;
			}
			PeViewer pe_viewer()
			{
				return {m_data.addr(), m_is_in_file};
			}
#pragma region 修改
			//添加一个段
			void add_section(const utils::data & section, const std::string & name, DWORD Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA)
			{
				auto & nt_headers = const_cast<IMAGE_NT_HEADERS &>(pe_viewer().get_nt_headers());
				auto section_table = const_cast<IMAGE_SECTION_HEADER*>(pe_viewer().get_section_table());
				
				//简单检查
				utils_assert(nt_headers.OptionalHeader.SizeOfImage % nt_headers.OptionalHeader.SectionAlignment == 0);
				utils_assert(name.length() < 8);

				uint64_t size = pe_viewer().get_section_table_iterator().size();
				strcpy((char*)section_table[size].Name, name.c_str());
				section_table[size].Misc.VirtualSize = section.size();
				section_table[size].VirtualAddress = nt_headers.OptionalHeader.SizeOfImage;
				section_table[size].SizeOfRawData = section.size();
				section_table[size].PointerToRawData = m_data.size();
				section_table[size].PointerToRelocations = 0;
				section_table[size].PointerToLinenumbers = 0;
				section_table[size].NumberOfRelocations = 0;
				section_table[size].NumberOfLinenumbers = 0;
				section_table[size].Characteristics = Characteristics;

				nt_headers.FileHeader.NumberOfSections ++;
				nt_headers.OptionalHeader.SizeOfImage += section.size();
				nt_headers.OptionalHeader.SizeOfImage = utils::math::round::bigger(nt_headers.OptionalHeader.SizeOfImage, nt_headers.OptionalHeader.SectionAlignment);

				m_data.append(section);
			}
#pragma endregion
		};
	}
	//PE编辑器
	using PeEditor = utils::MarkedLife<inner::PeEditor>;
#endif

#pragma region api集
	//获取api集的映射
	inline std::map<std::string, std::string> get_api_set_map(const std::string & filename = R"(C:\Windows\System32\apisetschema.dll)")
	{
		//refer: https://blog.quarkslab.com/runtime-dll-name-resolution-apisetschema-part-i.html
		//refer: https://blog.quarkslab.com/runtime-dll-name-resolution-apisetschema-part-ii.html

		//获取.apiset段
		auto data = [&]()
		{
			utils::FileReader input{filename};
			auto data_0 = input.read_all();

			utils::windows::pe::PeViewer pe_viewer{data_0.addr(), true};

			utils::CountChecker count_checker{1};
			utils::data data_1;
			for (const auto & one : pe_viewer.get_section_table_iterator())
			{
				if ((const char*)one.Name == std::string{".apiset"})
				{
					count_checker.count();

					//简单检查
					utils_assert(one.SizeOfRawData > 0);

					data_1 = data_0.subdata(one.PointerToRawData, one.SizeOfRawData);
				}
			}
			return data_1;
		}();

#pragma pack(push,1)
		struct s_header
		{
			uint32_t version{};
			uint32_t size{};
			uint32_t unknown_0{};
			uint32_t n{};
			uint32_t unknown_2{};
			uint32_t unknown_3{};
			uint32_t unknown_4{};
		};
		struct s_string
		{
			uint32_t text{};
			uint32_t size{};
		};
		struct s_entry
		{
			uint32_t unknown_maybe_is_exist_dll{};	//0 没有dll和其对应  1 有
			s_string name{};
			uint32_t unknown_1{};
			uint32_t dlls{};
			uint32_t dlls_n{};
		};
		struct s_dll
		{
			uint32_t unknown_0{};
			s_string from{};
			s_string to{};
		};
#pragma pack(pop)

		utils::DataReader reader{data};
		s_header header{};
		reader.read(header);

		//简单检查
		utils_assert(header.version == 0x06);
		utils_assert(header.unknown_0 == 0x0);
		utils_assert(header.n > 0);
		utils_assert(header.size < data.size());

		std::vector<s_entry> entries;
		reader.read(entries, header.n);

		auto get_text = [&data](const s_string & s) -> std::string
		{
			//简单检查
			utils_assert(s.text + s.size <= data.size());

			std::u16string u16;
			u16.assign(
				(const char16_t*)((uint64_t)data.addr() + s.text), utils::basic::exactly_divide(s.size, 2));
			return (const char*)utils::text::encoding::unicode::utf16_to_utf8(u16).c_str();
		};

		auto tmp = &data[0];

		std::map<std::string, std::string> api_set_map;
		for (const auto & one : entries)
		{
			//简单检查
			utils_assert(one.unknown_maybe_is_exist_dll == 0x1 || one.unknown_maybe_is_exist_dll == 0x0);
			utils_assert(one.dlls_n >= 1);
			utils_assert(one.dlls + sizeof(s_dll) * one.dlls_n <= data.size());

			auto name = get_text(one.name);

			auto dlls = (const s_dll*)((uint64_t)data.addr() + one.dlls);
			for (uint64_t i = 0; i < one.dlls_n; i++)
			{
				auto & dll = dlls[i];

				//简单检查
				utils_assert(dll.unknown_0 == 0x0);
				if (i == 0)
				{
					utils_assert(dll.from.text == 0x0);
					utils_assert(dll.from.size == 0x0);
				}
				else
				{
					utils_assert(dll.from.text == dlls[i - 1].to.text);
					utils_assert(dll.from.size == dlls[i - 1].to.size);
				}

				//跳过目标dll名字为空的
				if (dll.to.text == 0)
				{
					//简单检查
					utils_assert(dll.to.size == 0);

					continue;
				}

				//仅给出最后一个
				if (i == one.dlls_n - 1)
				{
					auto dll_name = get_text(dll.to);
					utils::map(api_set_map).insert_not_exist(name, dll_name);
				}
			}
		}
		return api_set_map;
	}

	//是否符合api集的命名规范
	inline bool is_api_set_name(const std::string & name)
	{
		return std::regex_match(name, std::regex{"(api-|ext-).*l[0-9]+-[0-9]+-[0-9]+"});
	}
#pragma endregion
}