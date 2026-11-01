#pragma once

#include <windows.h>
#include <utils/windows/process.hpp>
#include <utils/windows/pe.hpp>
#include <utils/inner/windows/dump.hpp>
#include <utils/DataWriter.hpp>
#include <utils/DataReader.hpp>

namespace utils::windows::dump
{
	//进程空间中的所有dll的导出表
	class ExportTables : public utils::inner::windows::dump::Base<ExportTables>
	{
	public:
		struct s_func_name
		{
			std::string dll_base_name;
			std::string func_name;
		};
	private:
		struct s_exported_function
		{
			std::string dll_base_name;
			std::string func_name;
			bool is_forward{};
			uint64_t addr{};
			std::string forward_dll;
			std::string forward_func_name;

			template<bool is_saving, typename T>
			void serialize(T& file)
			{
				file & dll_base_name;
				file & func_name;
				file & is_forward;
				file & addr;
				file & forward_dll;
				file & forward_func_name;
			}
		};
	private:
		utils::map<uint64_t, std::vector<s_func_name>> m_map_addr;
	public:
		//根据一个函数地址，反查是哪个dll的哪个函数
		//return: 如果没有找到, 返回空
		const std::vector<s_func_name> & get_name_by_addr(uint64_t addr) const
		{
			static std::vector<s_func_name> empty;

			if (m_map_addr.is_exist(addr))
				return m_map_addr[addr];
			else
				return empty;
		}
	private:
		friend class utils::inner::windows::dump::Base<ExportTables>;

		ExportTables()
		{
		}
		//收集信息
		//skip_dlls: 跳过的dll
		static utils::data gather(const std::vector<std::string> & skip_dlls)
		{
			std::vector<s_exported_function> exported_functions;
			for (const auto & one : utils::windows::process::get_dlls().m_dlls)
			{
				if ([&]()
				{
					for (const auto & skip : skip_dlls)
					{
						if (_stricmp(one.base_name.c_str(), skip.c_str()) == 0)
							return false;
					}
					return true;
				}())
				{
					utils::windows::pe::PeViewer pe_viewer{(void*)one.base, false};
					for (const auto & one_func : pe_viewer.get_export_table_detail())
					{
						exported_functions.push_back({
							one.base_name, one_func.name,
							one_func.is_forward, one_func.is_forward ? 0 : pe_viewer.get_by_rva(one_func.rva),
							one_func.forward_dll, one_func.forward_func_name});
					}
				}
			}

			DataWriter writer;
			writer.serializer & exported_functions;
			utils::data data;
			writer.get_data_and_clear(data);
			return data;
		}
		//执行分析
		static ExportTables analysis(utils::data && gather_data)
		{
			std::vector<s_exported_function> exported_functions;
			{
				DataReader reader{gather_data};
				reader.serializer & exported_functions;
			}

			static const auto api_set_map = utils::windows::pe::get_api_set_map(R"(C:\Windows\System32\apisetschema.dll)");

			utils::map<uint64_t, std::vector<s_func_name>> map_addr;
			std::function<std::optional<uint64_t>(const s_exported_function & one)> get_addr;
			get_addr = [&exported_functions, &get_addr](const s_exported_function & one)->std::optional<uint64_t>
			{
				if (one.is_forward)
				{
					uint64_t addr{};
					auto find_in = [&](const std::string & forward_dll)
					{
						for (const auto & other : exported_functions)
						{
							if (_stricmp(other.dll_base_name.c_str(), forward_dll.c_str()) == 0 &&
								other.func_name == one.forward_func_name)
							{
								auto _addr = get_addr(other);
								if (_addr.has_value())
								{
									addr = _addr.value();
									return true;
								}
								else
									return false;
							}
						}
						return false;
					};

					//普通查找
					if (find_in(one.forward_dll + ".dll")) return addr;

					//使用api集
					auto api_set_dll = [&]()->std::optional<std::string>
					{
						const auto & name = one.forward_dll;
						if (api_set_map.is_exist(name)) return api_set_map[name];

						if (!utils::windows::pe::is_api_set_name(name)) return{};

						//找最小版本号能匹配的

						auto split_name = [](std::string & prefix, uint64_t & version_c, const std::string & name)
						{
							auto pos = name.rfind('-');

							//简单检查
							utils_assert(pos != name.npos);

							prefix = name.substr(0, pos);
							utils::text::value::from_text(version_c, name.substr(pos + 1));
						};

						std::string prefix;
						uint64_t version_c{};
						split_name(prefix, version_c, name);

						utils::CountChecker count_checker{0,1};
						std::string result;
						for (const auto & one_map : api_set_map)
						{
							std::string _prefix;
							uint64_t _version_c{};
							split_name(_prefix, _version_c, one_map.first);

							if (prefix == _prefix && version_c <= _version_c)
							{
								//简单检查
								utils_assert(version_c != _version_c);

								count_checker.count();
								result = one_map.second;
							}
						}
						if (count_checker.get_count() > 0) return result;

						//引用的dll或函数不存在
						return {};
					}();
					if (api_set_dll.has_value() && find_in(api_set_dll.value())) return addr;

					//引用的函数不存在
					return {};
				}
				else
					return one.addr;
			};
			for (const auto & one : exported_functions)
			{
				auto addr = get_addr(one);
				if (addr.has_value())
				{
					//简单检查
					utils_assert(addr.value() != 0);

					map_addr[addr.value()].push_back({one.dll_base_name, one.func_name});
				}
			}

			//给出
			ExportTables one;
			one.m_map_addr = std::move(map_addr);
			return one;
		}
	};
}