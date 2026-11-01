#pragma once

#include <windows.h>
#include <utils/windows/undocumented.hpp>
#include <utils/windows/pe.hpp>
#include <utils/windows/process.hpp>

//refer: https://doxygen.reactos.org/d8/d2f/unwind_8c.html

#if 0	//todo


使用新式解析，然后和旧解析对照
#endif

namespace utils::windows::unwind
{
	using namespace utils::windows::undocumented::unwind;
	
#pragma region 较底层操作
	//获得指定的unwind_code占用了几个槽
	inline uint64_t get_slot_n_of_one_unwind_code(const UNWIND_CODE & unwind_code)
	{
		switch(unwind_code.UnwindOp)
		{
		case UWOP_PUSH_NONVOL: return 1;
		case UWOP_ALLOC_LARGE:
			if (unwind_code.OpInfo == 0) return 2;
			if (unwind_code.OpInfo == 1) return 3;
			utils_throw();
		case UWOP_ALLOC_SMALL: return 1;
		case UWOP_SET_FPREG: return 1;
		case UWOP_SAVE_NONVOL: return 2;
		case UWOP_SAVE_NONVOL_FAR: return 3;
		case UWOP_EPILOG: return 1;
		case UWOP_SAVE_XMM128: return 2;
		case UWOP_SAVE_XMM128_FAR: return 3;
		case UWOP_PUSH_MACHFRAME: return 1;
		}
		utils_throw();
	}
	//获得unwind_info中, unwind_codes之后的信息
	inline const void* get_member_after_unwind_codes(const UNWIND_INFO & unwind_info)
	{
		uint64_t code_n = unwind_info.CountOfCodes;
		if(code_n % 2 == 1) code_n++;
		return &unwind_info.UnwindCode[code_n];
	}
	//获得下一个RUNTIME_FUNCTION
	inline const RUNTIME_FUNCTION & get_next_runtime_function(const UNWIND_INFO & unwind_info)
	{
		//简单检查
		utils_assert(unwind_info.Flags & UNW_FLAG_CHAININFO);

		return *(const RUNTIME_FUNCTION*)get_member_after_unwind_codes(unwind_info);
	}
	//遍历每个unwind_code
	inline void for_each_unwind_code(const UNWIND_INFO & unwind_info, uint64_t i_begin,
		std::function<void(uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)> meet_one_unwind_code)
	{
		for(uint64_t i = i_begin; i < unwind_info.CountOfCodes; )
		{
			bool is_stop{};
			meet_one_unwind_code(i, unwind_info.UnwindCode[i], is_stop);
			if(is_stop) break;

			uint64_t size = get_slot_n_of_one_unwind_code(unwind_info.UnwindCode[i]);
			i += size;
			
			//简单检查
			utils_assert(i <= unwind_info.CountOfCodes);
		}
	}
	//沿着链, 遍历每个unwind_code
	inline void for_each_unwind_code_in_chain(
		const UNWIND_INFO & unwind_info,
		const utils::windows::pe::PeViewer & pe_viewer,
		std::function<void(uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)> meet_one_unwind_code)
	{
		auto p_unwind_info = &unwind_info;
		for(uint64_t j = 0; ; j++)
		{
			bool _is_stop{};
			for_each_unwind_code(*p_unwind_info, 0, [&meet_one_unwind_code, &_is_stop, p_unwind_info, j](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
			{
				meet_one_unwind_code(i, j, unwind_code, *p_unwind_info, is_stop);
				_is_stop = is_stop;
			});
			if(_is_stop) break;

			if (!(p_unwind_info->Flags & UNW_FLAG_CHAININFO))
				break;

			auto & next_one = get_next_runtime_function(*p_unwind_info);
			p_unwind_info = pe_viewer.get_unwind_info(next_one);
		}
	}
	//沿着链, 遍历每个函数块
	inline void for_each_runtime_function_in_chain(
		const RUNTIME_FUNCTION & start,
		const utils::windows::pe::PeViewer & pe_viewer,
		std::function<void(uint64_t j, const RUNTIME_FUNCTION & runtime_function)> meet_one_runtime_function)
	{
		auto p_runtime_function = &start;
		for(uint64_t j = 0; ; j++)
		{
			meet_one_runtime_function(j, *p_runtime_function);

			auto p_unwind_info = pe_viewer.get_unwind_info(*p_runtime_function);
			if (!(p_unwind_info->Flags & UNW_FLAG_CHAININFO))
				return;

			auto & next_one = get_next_runtime_function(*p_unwind_info);
			p_runtime_function = &next_one;
		}
	}
	//获得链的最后一个函数块
	const RUNTIME_FUNCTION & get_last_runtime_function_in_chain(
		const RUNTIME_FUNCTION & runtime_function,
		const utils::windows::pe::PeViewer & pe_viewer)
	{
		auto p_runtime_function = &runtime_function;
		while(1)
		{
			auto p_unwind_info = pe_viewer.get_unwind_info(*p_runtime_function);
			if (!(p_unwind_info->Flags & UNW_FLAG_CHAININFO))
				return *p_runtime_function;

			auto & next_one = get_next_runtime_function(*p_unwind_info);
			p_runtime_function = &next_one;
		}
	}
	//获得epilog的大小
	inline uint64_t get_epilog_size(const UNWIND_INFO & unwind_info)
	{
		//简单检查
		utils_assert(unwind_info.CountOfCodes >= 1);
		utils_assert(unwind_info.UnwindCode[0].UnwindOp == UWOP_EPILOG);

		return unwind_info.UnwindCode[0].CodeOffset;
	}
	//获得描述epilog位置的首个code的索引
	inline uint64_t get_first_EpilogPos_index(const UNWIND_INFO & unwind_info)
	{
		//简单检查
		utils_assert(unwind_info.CountOfCodes >= 1);
		utils_assert(unwind_info.UnwindCode[0].UnwindOp == UWOP_EPILOG);

		if(unwind_info.UnwindCode[0].OpInfo == 1)
			return 0;
		
		//简单检查
		utils_assert(unwind_info.UnwindCode[0].OpInfo == 0);
		utils_assert(unwind_info.CountOfCodes >= 2);
		utils_assert(unwind_info.UnwindCode[1].UnwindOp == UWOP_EPILOG);

		return 1;
	}
	//获得epilog位置, (此位置指 距离RUNTIME_FUNCTION的尾部)
	//i: unwind_code在unwind_codes数组中的索引
	inline uint64_t get_epilog_offset(const UNWIND_CODE & unwind_code, uint64_t i)
	{
		//简单检查
		utils_assert(unwind_code.UnwindOp == UWOP_EPILOG);

		if (i == 0)
			return (uint64_t)unwind_code.CodeOffset;
		else
			return (uint64_t)unwind_code.CodeOffset + ((uint64_t)unwind_code.OpInfo << 8);
	}
	//根据pc的rva，获得所在函数块
	//返回值: 如果没找到，返回nullptr
	inline const RUNTIME_FUNCTION* get_runtime_function(uint64_t rva, const utils::windows::pe::PeViewer & pe_viewer)
	{
		//获得所有函数块
		const auto range = pe_viewer.get_runtime_functions_iterator();

		//二分查找
		const auto iter = std::lower_bound(range.begin(), range.end(), rva,
			[](const RUNTIME_FUNCTION & one, uint64_t rva) -> bool
		{
			return one.EndAddress <= rva;
		});
			
		//没有找到
		if (iter == range.end()) return {};
		if (!(rva >= iter->BeginAddress && rva < iter->EndAddress)) return {};

		//找到了
		return &*iter;
	}
#pragma endregion

#pragma region 较高层操作
	//检查所有的unwind_info，看看是否符合我的认知
	inline void check_unwind_info(const utils::windows::pe::PeViewer & pe_viewer)
	{
		//简单检查: 64位文件
		utils_assert(pe_viewer.is_x64());

		//目录和.pdata同时不存在，则不再检查
		{
			const auto & directory_entry = pe_viewer.get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
			
			if(directory_entry.VirtualAddress == 0 && directory_entry.Size == 0)
			{
				//简单检查
				utils_assert(!pe_viewer.is_exist_section(u8".pdata"));

				//检查完毕
				return;
			}
		}
		//简单检查: 对.pdata段的检查，则.pdata后边，在目录中没用到的，都是0
		if(pe_viewer.is_exist_section(u8".pdata"))
		{
			const auto & directory_entry = pe_viewer.get_directory_entry(IMAGE_DIRECTORY_ENTRY_EXCEPTION);
			
			const auto & section = pe_viewer.get_section_by_name(u8".pdata");
				
			//简单检查: .pdata段和目录中的数据, 开头一致
			utils_assert(directory_entry.VirtualAddress == section.VirtualAddress);
			utils_assert(directory_entry.Size <= section.Misc.VirtualSize);

			//简单检查：.pdata段后边多出来的，都是0
			if(directory_entry.Size < section.Misc.VirtualSize)
			{
				auto p = pe_viewer.get_by_rva<uint8_t*>(
					directory_entry.VirtualAddress, section.Misc.VirtualSize);

				for(uint64_t i = 0; i < section.Misc.VirtualSize - directory_entry.Size;i++)
				{
					auto addr = p + directory_entry.Size + i;

					utils_assert(*addr == 0);
				}
			}
		}
		//简单检查: 都有值, 并且不越界
		for (const auto & one : pe_viewer.get_runtime_functions_iterator())
		{
			//跳过空值
			if (!one.BeginAddress)
			{
				utils_assert(!one.EndAddress);
				utils_assert(!one.UnwindData);
				continue;
			}

			utils_assert(one.EndAddress);
			utils_assert(one.UnwindData);
			utils_assert(one.BeginAddress <= one.EndAddress);
			utils_assert(one.EndAddress <= pe_viewer.get_module_size());
			utils_assert(one.UnwindData <= pe_viewer.get_module_size());
		}
		//简单检查: runtime_functions按顺序排列
		{
			RUNTIME_FUNCTION last_one{};
			for(const auto & one : pe_viewer.get_runtime_functions_iterator())
			{
				//跳过空值
				if (!one.BeginAddress) continue;

				//第一次有值
				if (!last_one.BeginAddress)
				{
					last_one = one;
					continue;
				}

				utils_assert(last_one.EndAddress <= one.BeginAddress);
					
				last_one = one;
			}
		}
		//简单检查: 检查每个函数块
		for(const auto & one : pe_viewer.get_runtime_functions_iterator())
		{
			//跳过空值
			if (!one.BeginAddress) continue;

			const auto unwind_info = pe_viewer.get_unwind_info(one);

			//简单检查: Version字段
			utils_assert(unwind_info->Version == 1 || unwind_info->Version == 2);

			//简单检查: Flags字段, 只有三种取值
			utils_assert(0 == (unwind_info->Flags & ~(UNW_FLAG_CHAININFO | UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER)));

			//简单检查: 链和错误处理，不同时存在
			if(unwind_info->Flags & UNW_FLAG_CHAININFO)
			{
				utils_assert(0==(unwind_info->Flags & UNW_FLAG_EHANDLER));
				utils_assert(0==(unwind_info->Flags & UNW_FLAG_UHANDLER));
			}

			//简单检查: SizeOfProlog字段，不大过函数块的大小
			utils_assert(unwind_info->SizeOfProlog <= one.EndAddress - one.BeginAddress);

			//简单检查: 使用了FrameRegister才会使用FrameOffset
			if(unwind_info->FrameOffset)
			{
				utils_assert(unwind_info->FrameRegister);
			}
			//简单检查: UnwindCodes末尾，填充的0
			if(unwind_info->CountOfCodes % 2 == 1)
			{
				utils_assert(unwind_info->CountOfCodes != 0xff);
				utils_assert(utils::basic::is_zero(unwind_info->UnwindCode[unwind_info->CountOfCodes]));
			}
			//简单检查: 遍历后，i是否正好是unwind_info->CountOfCodes
			{
				uint64_t i = 0;
				for(; i < unwind_info->CountOfCodes; i++)
				{
					switch(unwind_info->UnwindCode[i].UnwindOp)
					{
					case UWOP_PUSH_NONVOL:
						break;
					case UWOP_ALLOC_LARGE:
					{
						const auto OpInfo = unwind_info->UnwindCode[i].OpInfo;

						if(OpInfo == 0)
						{
							i++;
							utils_assert(i < unwind_info->CountOfCodes);
						}
						else if(OpInfo == 1)
						{
							i+=2;
							utils_assert(i < unwind_info->CountOfCodes);
						}
						else
							utils_throw();

						break;
					}
					case UWOP_ALLOC_SMALL:
						break;
					case UWOP_SET_FPREG: 
						break;
					case UWOP_SAVE_NONVOL:
						i++;
						utils_assert(i < unwind_info->CountOfCodes);
						break;
					case UWOP_SAVE_NONVOL_FAR:  
						i+=2;
						utils_assert(i < unwind_info->CountOfCodes);
						break;
					case UWOP_EPILOG:
						break;
					case UWOP_SAVE_XMM128:
						i++;
						utils_assert(i < unwind_info->CountOfCodes);
						break;
					case UWOP_SAVE_XMM128_FAR:
						i+=2;
						utils_assert(i < unwind_info->CountOfCodes);
						break;
					case UWOP_PUSH_MACHFRAME:
						break;
					default:
						utils_throw();
					}
				}
				utils_assert(i == unwind_info->CountOfCodes);
			}
			//简单检查: UnwindCode.CodeOffset字段，是否过大
			utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
				[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
				{
					if(unwind_code.UnwindOp == UWOP_EPILOG)
					{
						;	//由接下来的代码来检查
					}
					else
					{
						utils_assert(unwind_code.CodeOffset <= unwind_info->SizeOfProlog);
					}
				});
			//简单检查: UWOP_EPILOG: 只出现在v2中
			utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
				[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_EPILOG:
						utils_assert(unwind_info->Version == 2);
						break;
					}
				});
			//简单检查: UWOP_PUSH_MACHFRAME: OpInfo只有0和1
			utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
				[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_PUSH_MACHFRAME:
						utils_assert(unwind_code.OpInfo == 1 || unwind_code.OpInfo == 0);
						break;
					}
				});
			//简单检查: 链上的节点，或者都使用v1，或者都使用v2
			{
				uint64_t versions{};
				utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
					[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
				{
					versions |= unwind_info.Version;
				});
			}
			//简单检查: 链上的顺序关系
			{
#if 0   //版本2
				SAVE/FPREG
				ALLOC_LARGE/ALLOC_SMALL
				PUSH
				ALLOC_SMALL
				MACHFRAME
#endif
#if 0   //版本1
				SAVE/PUSH/ALLOC_LARGE/ALLOC_SMALL/FPREG
				MACHFRAME
#endif
				std::vector<uint64_t> is_meet(11, 0);
				//简单检查: 顺序关系
				if (unwind_info->Version == 1)
				{
					utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
						[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
					{
						switch(unwind_code.UnwindOp)
						{
						case UWOP_PUSH_MACHFRAME:
							is_meet[UWOP_PUSH_MACHFRAME] ++;
							break;
						case UWOP_PUSH_NONVOL:
						case UWOP_ALLOC_LARGE:
						case UWOP_ALLOC_SMALL:
						case UWOP_SAVE_NONVOL:
						case UWOP_SAVE_NONVOL_FAR:
						case UWOP_SAVE_XMM128:
						case UWOP_SAVE_XMM128_FAR:
						case UWOP_SET_FPREG:
							is_meet[UWOP_PUSH_NONVOL] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							break;
						case UWOP_EPILOG:
							break;
						default:
							utils_throw();
						}
					});
				}
				else if (unwind_info->Version == 2)
				{
					utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
						[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
					{
						switch(unwind_code.UnwindOp)
						{
						case UWOP_PUSH_MACHFRAME:
							is_meet[UWOP_PUSH_MACHFRAME] ++;
							break;
						case UWOP_ALLOC_SMALL:
							if(is_meet[UWOP_PUSH_NONVOL])
							{
								utils_assert(unwind_code.OpInfo == 0);

								is_meet[UWOP_ALLOC_SMALL] ++;
								utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							}
							else
							{
								is_meet[UWOP_ALLOC_LARGE] ++;
								utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
								utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
								utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							}
							break;
						case UWOP_PUSH_NONVOL:
							is_meet[UWOP_PUSH_NONVOL] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							break;
						case UWOP_SAVE_NONVOL:
						case UWOP_SAVE_NONVOL_FAR:
						case UWOP_SAVE_XMM128:
						case UWOP_SAVE_XMM128_FAR:
						case UWOP_ALLOC_LARGE:
						case UWOP_SET_FPREG:
							is_meet[UWOP_SAVE_NONVOL] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							break;
						case UWOP_EPILOG:
							break;
						default:
							utils_throw();
						}
					});
#if 0
					utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
						[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
					{
						switch(unwind_code.UnwindOp)
						{
						case UWOP_PUSH_MACHFRAME:
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							is_meet[UWOP_PUSH_MACHFRAME] ++;
							break;
						case UWOP_PUSH_NONVOL:
							is_meet[UWOP_PUSH_NONVOL] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							break;
						case UWOP_ALLOC_LARGE:
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							is_meet[UWOP_ALLOC_LARGE] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							break;
						case UWOP_ALLOC_SMALL:
							if(is_meet[UWOP_PUSH_NONVOL])
							{
								utils_assert(unwind_code.OpInfo == 0);
								utils_assert(is_meet[UWOP_ALLOC_SMALL] <= 1);
								utils_assert(!is_meet_2[UWOP_ALLOC_SMALL]);
								is_meet_2[UWOP_ALLOC_SMALL] ++;
							}
							else
							{
								if(is_meet[UWOP_ALLOC_SMALL])
								{
									utils_assert(is_meet[UWOP_ALLOC_SMALL] == 1);
									utils_assert(!is_meet_2[UWOP_ALLOC_SMALL]);
									utils_assert(unwind_code.OpInfo == 0);
									is_meet_2[UWOP_ALLOC_SMALL] ++;
								}
								else
								{
									utils_assert(!is_meet_2[UWOP_ALLOC_SMALL]);
									is_meet[UWOP_ALLOC_SMALL] ++;
									utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
								}
							}
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							break;
						case UWOP_SET_FPREG:
							utils_assert(!is_meet[UWOP_SET_FPREG]);
							is_meet[UWOP_SET_FPREG] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							if(unwind_info.Version == 2)
							{
								utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							}
							break;
						case UWOP_SAVE_NONVOL:
							is_meet[UWOP_SAVE_NONVOL] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							//utils_assert(!is_meet[UWOP_SET_FPREG]);
							break;
						case UWOP_SAVE_NONVOL_FAR:
							is_meet[UWOP_SAVE_NONVOL_FAR] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							utils_assert(!is_meet[UWOP_SET_FPREG]);
							break;
						case UWOP_SAVE_XMM128:
							is_meet[UWOP_SAVE_XMM128] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							utils_assert(!is_meet[UWOP_SET_FPREG]);
							break;
						case UWOP_SAVE_XMM128_FAR:
							is_meet[UWOP_SAVE_XMM128_FAR] ++;
							utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							utils_assert(!is_meet[UWOP_SET_FPREG]);
							break;
						case UWOP_EPILOG:
							is_meet[UWOP_EPILOG] ++;
							//utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
							//utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
							//utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							//utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							//utils_assert(!is_meet[UWOP_SET_FPREG]);
							//utils_assert(!is_meet[UWOP_SAVE_NONVOL]);
							//utils_assert(!is_meet[UWOP_SAVE_NONVOL_FAR]);
							//utils_assert(!is_meet[UWOP_SAVE_XMM128]);
							//utils_assert(!is_meet[UWOP_SAVE_XMM128_FAR]);
							break;
						default:
							utils_throw();
						}
					});
#endif
				}
				else
					utils_throw();
				//简单检查: 个数限制
				is_meet.clear(); is_meet.resize(11);
				utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
					[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_PUSH_MACHFRAME:
						utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
						is_meet[UWOP_PUSH_MACHFRAME] ++;
						break;
					case UWOP_SET_FPREG:
						utils_assert(!is_meet[UWOP_SET_FPREG]);
						is_meet[UWOP_SET_FPREG] ++;
						break;
					case UWOP_ALLOC_LARGE:
					case UWOP_ALLOC_SMALL:
					case UWOP_SAVE_NONVOL:
					case UWOP_SAVE_NONVOL_FAR:
					case UWOP_SAVE_XMM128:
					case UWOP_SAVE_XMM128_FAR:
					case UWOP_PUSH_NONVOL:
					case UWOP_EPILOG:
						break;
					default:
						utils_throw();
					}
				});
				//简单检查: 只开一次栈
				if (unwind_info->Version == 2)
				{
					is_meet.clear(); is_meet.resize(11);
					utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
						[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
					{
						switch(unwind_code.UnwindOp)
						{
						case UWOP_ALLOC_LARGE:
							utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
							is_meet[UWOP_ALLOC_LARGE] ++;
							break;
						case UWOP_ALLOC_SMALL:
							//UWOP_PUSH_NONVOL后面的当做popf，而不是开栈
							if(!is_meet[UWOP_PUSH_NONVOL])
							{
								//把这个当做开栈
								if(!is_meet[UWOP_ALLOC_LARGE])
								{
									is_meet[UWOP_ALLOC_LARGE] ++;
								}
								//把这个当做popf
								else
								{
									utils_assert(unwind_code.OpInfo == 0);
									utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
									is_meet[UWOP_ALLOC_SMALL] ++;
								}
							}
							else
							{
								utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							}
							break;
						case UWOP_PUSH_NONVOL:
							is_meet[UWOP_PUSH_NONVOL] ++;
							break;
						case UWOP_SET_FPREG:
						case UWOP_PUSH_MACHFRAME:
						case UWOP_SAVE_NONVOL:
						case UWOP_SAVE_NONVOL_FAR:
						case UWOP_SAVE_XMM128:
						case UWOP_SAVE_XMM128_FAR:
						case UWOP_EPILOG:
							break;
						default:
							utils_throw();
						}
					});
				}
				//简单检查: 最多只有一个popf
				if (unwind_info->Version == 2)
				{
					is_meet.clear(); is_meet.resize(11);
					utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
						[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_ALLOC_SMALL:
						if(is_meet[UWOP_PUSH_NONVOL])
						{
							utils_assert(unwind_code.OpInfo == 0);
							utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
							is_meet[UWOP_ALLOC_SMALL] ++;
						}
						break;
					case UWOP_PUSH_NONVOL:
						is_meet[UWOP_PUSH_NONVOL] ++;
						break;
					case UWOP_SET_FPREG:
					case UWOP_PUSH_MACHFRAME:
					case UWOP_ALLOC_LARGE:
					case UWOP_SAVE_NONVOL:
					case UWOP_SAVE_NONVOL_FAR:
					case UWOP_SAVE_XMM128:
					case UWOP_SAVE_XMM128_FAR:
					case UWOP_EPILOG:
						break;
					default:
						utils_throw();
					}
				});
				}
			}
			//简单检查: 链上每个节点中的顺序关系: UWOP_EPILOG在每个节点的最前
			utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
				[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
			{
				//不是新的节点
				if(i != 0) return;
						
				std::vector<uint64_t> is_meet(11, 0);

				utils::windows::unwind::for_each_unwind_code(unwind_info, 0, [&is_meet](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_PUSH_MACHFRAME:
					case UWOP_PUSH_NONVOL:
					case UWOP_ALLOC_LARGE:
					case UWOP_ALLOC_SMALL:
					case UWOP_SET_FPREG:
					case UWOP_SAVE_NONVOL:
					case UWOP_SAVE_NONVOL_FAR:
					case UWOP_SAVE_XMM128:
					case UWOP_SAVE_XMM128_FAR:
						is_meet[unwind_code.UnwindOp] ++;
						break;
					case UWOP_EPILOG:
						is_meet[UWOP_EPILOG] ++;
						utils_assert(!is_meet[UWOP_PUSH_MACHFRAME]);
						utils_assert(!is_meet[UWOP_PUSH_NONVOL]);
						utils_assert(!is_meet[UWOP_ALLOC_SMALL]);
						utils_assert(!is_meet[UWOP_ALLOC_LARGE]);
						utils_assert(!is_meet[UWOP_SET_FPREG]);
						utils_assert(!is_meet[UWOP_SAVE_NONVOL]);
						utils_assert(!is_meet[UWOP_SAVE_NONVOL_FAR]);
						utils_assert(!is_meet[UWOP_SAVE_XMM128]);
						utils_assert(!is_meet[UWOP_SAVE_XMM128_FAR]);
						break;
					default:
						utils_throw();
					}
				});
			});
			//简单检查: FrameRegister的信息在链上是相同的
			{
				UBYTE FrameRegister{};
				UBYTE FrameOffset{};
				bool is_first = true;
				utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
					[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
				{
					if(is_first)
					{
						is_first = false;
						
						FrameRegister = unwind_info.FrameRegister;
						FrameOffset = unwind_info.FrameOffset;
					}
					else
					{
						utils_assert(FrameRegister == (UBYTE)unwind_info.FrameRegister);
						utils_assert(FrameOffset == (UBYTE)unwind_info.FrameOffset);
					}
				});
			}
			//针对版本2时的检查
			if(unwind_info->Version == 2)
			{
				//简单检查: UWOP_EPILOG的个数是偶数
				{
					uint64_t n{};
					utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
						[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
						{
							switch(unwind_code.UnwindOp)
							{
							case UWOP_EPILOG:
								n++;
								break;
							}
						});
					utils_assert(n % 2 == 0);
				}
				//简单检查: 第一个UWOP_EPILOG的OpInfo只能是0或1
				utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
					[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
					{
						if(i == 0 && unwind_code.UnwindOp == UWOP_EPILOG)
						{
							utils_assert(unwind_code.OpInfo == 1 || unwind_code.OpInfo == 0);
						}
						is_stop = true;
					});
				//简单检查: UWOP_EPILOG的CodeOffset字段: 不能过大
				utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
					[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
					{
						if(unwind_code.UnwindOp == UWOP_EPILOG)
						{
							const auto offset = utils::windows::unwind::get_epilog_offset(unwind_code, i);
							utils_assert(offset <= one.EndAddress - one.BeginAddress);
						}
						
						if(unwind_code.UnwindOp != UWOP_EPILOG)
							is_stop = true;
					});
				//简单检查: UWOP_EPILOG的地址是从小到大的，并且没有0，除非是最后一个
				{
					//计算UWOP_EPILOG的个数
					uint64_t n{};
					utils::windows::unwind::for_each_unwind_code(*unwind_info, 0,
						[&](uint64_t i, const UNWIND_CODE & unwind_code, bool & is_stop)
						{
							switch(unwind_code.UnwindOp)
							{
							case UWOP_EPILOG:
								n++;
								break;
							}
						});

					//检查
					if (n > 0)
					{
						for(uint64_t i = 0; i < n - 1; i++)
						{
							const auto & a = unwind_info->UnwindCode[i];
							const auto & b = unwind_info->UnwindCode[i+1];
					
							const auto offset_a = utils::windows::unwind::get_epilog_offset(a, i);
							const auto offset_b = utils::windows::unwind::get_epilog_offset(b, i+1);

							if(offset_b == 0)
							{
								utils_assert(i == n - 2);
								break;
							}

							utils_assert(offset_a < offset_b);
							utils_assert(offset_a > 0);
						}
					}
				}
			}
			//简单检查: 链上的节点都在模块的函数块列表里出现过
			utils::windows::unwind::for_each_runtime_function_in_chain(one, pe_viewer,
				[&](uint64_t j, const RUNTIME_FUNCTION & runtime_function)
			{
				//首节点已经在列表中了
				if (j == 0) return;
				
				//使用当前函数块的第一个地址，在模块的函数块列表里查找
				const auto in_list = utils::windows::unwind::get_runtime_function(runtime_function.BeginAddress, pe_viewer);

				//简单检查: 能找到
				utils_assert(in_list);
			});
			//简单检查: 使用了FrameRegister就一定有UWOP_SET_FPREG
			if(unwind_info->FrameOffset)
			{
				bool is_exist{};
				utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, pe_viewer,
					[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
				{
					switch(unwind_code.UnwindOp)
					{
					case UWOP_SET_FPREG:
						is_exist = true;
						is_stop = true;
						break;
					}
				});
				utils_assert(is_exist);
			}
		}
	}
	//执行对函数栈帧的unwind操作
	class StackUnwinder
	{
		//请设置
	public:
		utils::windows::process::ModuleInfoManager* m_module_info_manager{};
		std::function<void(uint64_t addr, void* buffer, uint64_t size)> m_read_bytes =
			[](uint64_t addr, void* buffer, uint64_t size){memmove(buffer,(const void*)addr, size);};

	public:
		//试着展开一层函数调用
		bool try_unwind_one_function(CONTEXT & context)
		{
#if 0
			//查找模块
			auto & module = m_module_info_manager->get_module_by_inside_addr(context.Rip);

			//额外数据的名字
			static const std::u8string runtime_functions_name =
				u8"utils::windows::unwind::StackUnwinder::runtime_functions";

			//没有在此模块展开过函数
			if (!module.has_more_data(runtime_functions_name))
			{
				//检查unwind数据
				utils::windows::unwind::check_unwind_info(*module.m_pe_viewer);

				//向模块添加数据
				module.add_more_data<m_runtime_functions_t>(
					runtime_functions_name, [&]()
				{
					auto iterator = module.m_pe_viewer->get_runtime_functions_iterator();

					//去掉空值
					std::vector<RUNTIME_FUNCTION> functions;
					for (const auto & one : iterator)
					{
						if (one.BeginAddress == 0) continue;

						functions.push_back(one);
					}
					return functions;
				}());
			}

			//todo 改为二分查找

			//获得额外数据
			auto & runtime_functions = module.get_more_data<m_runtime_functions_t>(runtime_functions_name);

			for(const auto & one : runtime_functions)
			{
				if(!utils::Range1D{module.m_base + one.BeginAddress, one.EndAddress - one.BeginAddress}.is_include(context.Rip))
					continue;
#endif
			//查找模块
			auto & module = m_module_info_manager->get_module_by_inside_addr(context.Rip);

			//查找函数块
			auto p_runtime_function = get_runtime_function_by_addr(context.Rip - module.m_base, module);

			//没有找到
			if (!p_runtime_function) return false;

			//转为引用
			const auto & one = *p_runtime_function;

			//不论哪个版本，逻辑都是：
			//1.检查是否在某个epilog里
			//2.否则, 执行所有的code，遍历链，（跳过链的第一个节点里，那些没执行的）
			//note: ntdll.dll中，通过逆向，似乎windows在这里的处理有点问题。
			//      它似乎对链的第二个和第二个往后的节点，也应用了比较，来跳过一些unwind_code.
			//      而这样的逻辑，似乎只应该对第一个节点使用，而不是第二个和第二个往后的节点，
			//      因为它们的所有准备工作都是做完了，自然要执行所有的恢复操作。但windows似乎
			//      应用了比较，打算跳过一些unwind_code。此处的比较，windows似乎在用第一个节点中
			//      算出来的，rip与第一个节点开头的距离，来和其他节点中的offset比较的

			auto unwind_info = module.m_pe_viewer->get_unwind_info(one);
			switch(unwind_info->Version)
			{
			case 1:
			{
				if(try_unwind_epilog_v1(context, module, one))
					return true;
					
				unwind_prolog_or_body_v1_v2(context, module, one);
				return true;
			}
			case 2:
			{
				if(try_unwind_epilog_v2(context, module, one))
					return true;
					
				unwind_prolog_or_body_v1_v2(context, module, one);
				return true;
			}
			}
			utils_throw();
#if 0
			}
			return false;
#endif
		}
		//获得调用栈
		std::vector<uint64_t> get_call_stack(const CONTEXT & context)
		{
			//复制一份context
			CONTEXT _context = context;

			std::vector<uint64_t> addrs;
			while(1)
			{
				if (try_unwind_one_function(_context))
				{
					addrs.push_back(_context.Rip);
					if(_context.Rip == 0)
						break;
				}
				else
				{
					break;
				}
			}
			return addrs;
		}
		//通过函数块中某个地址，获得函数入口地址
		//note: 如果没有函数块和地址对应，返回空值
		//note: 参数和返回值都是进程中的真实地址，不是rva
		std::optional<uint64_t> get_function_entry_point(uint64_t addr)
		{
			//查找模块
			auto & module = m_module_info_manager->get_module_by_inside_addr(addr);

			//查找函数块
			auto p_runtime_function = get_runtime_function_by_addr(addr - module.m_base, module);
			
			//没有找到
			if(!p_runtime_function) return {};

			//获得链上最后一个函数块
			auto & last_runtime_function = utils::windows::unwind::get_last_runtime_function_in_chain(
				*p_runtime_function, *module.m_pe_viewer);

			//给出
			return last_runtime_function.BeginAddress + module.m_base;
		}
	private:
#pragma region 小函数
		//获得还原save操作时的基地址
		//is_skiped_frame_register: 帧指针是否被跳过，i.e. PC在prolog中，还未执行到帧指针的赋值
		uint64_t get_save_base_addr(const UNWIND_INFO & unwind_info, const CONTEXT & context,
			bool is_skiped_frame_register)
		{
			//简单检查: 跳过标志只在有帧指针时才会被设置
			if (is_skiped_frame_register)
				utils_assert(unwind_info.FrameRegister);

			//有FrameRegister使用FrameRegister推出的rsp，没有直接使用rsp
			if (unwind_info.FrameRegister)
			{
				//帧指针还未被赋值，就需要unwind了。此时直接使用rsp来做基址。
				if (is_skiped_frame_register)
					return context.Rsp;

				uint64_t offset = unwind_info.FrameOffset; offset *= 16;
				auto rsp = get_reg_in_context(context, unwind_info.FrameRegister) - offset;
				return rsp;
			}
			else
			{
				return context.Rsp;
			}
		}
		//通过地址，获得函数块
		//addr: rva
		const RUNTIME_FUNCTION* get_runtime_function_by_addr(uint64_t addr,
			utils::windows::process::ModuleInfoManager::s_module & module)
		{
			//额外数据的名字
			static const std::u8string name_for_is_check_unwind_info =
				u8"utils::windows::unwind::StackUnwinder::is_check_unwind_info";

			//没有在此模块展开过函数
			if (!module.has_more_data(name_for_is_check_unwind_info))
			{
				//检查unwind数据
				utils::windows::unwind::check_unwind_info(*module.m_pe_viewer);

				//向模块添加数据
				module.add_more_data<bool>(name_for_is_check_unwind_info, true);
			}

			return utils::windows::unwind::get_runtime_function(addr, *module.m_pe_viewer);
		}
		//用于从rsp和rip附近读取数据
		//addr: 进程中的真实地址
		template<typename T>
		void read_bytes(T & value, uint64_t addr)
		{
			m_read_bytes(addr, &value, sizeof(T));
		}
		//解析pop指令
		bool try_decode_pop_opcode(const uint16_t & opcode_2B, uint64_t & reg)
		{
			//解析1字节pop指令
			if ( (opcode_2B & 0xf8) == 0x58 )
			{
				reg = opcode_2B & 0x7;
				return true;
			}
			//解析2字节pop指令
			if ( (opcode_2B & 0xf8f0) == 0x5840 )
			{
				//简单检查: rex的前缀是rex.b
				utils_assert((opcode_2B & 0xff) == 0x41);
				
				reg = ((opcode_2B >> 8) & 0x7) + 8;
				return true;
			}
			return false;
		}
		//模拟执行pop指令
		void do_pop(CONTEXT & context, uint64_t reg)
		{
			read_bytes(*(&context.Rax + reg), context.Rsp);
			context.Rsp+=8;
			context.Rip+=(reg < 8 ? 1 : 2);
		}
		//模拟执行ret指令
		void do_ret(CONTEXT & context)
		{
			read_bytes(context.Rip, context.Rsp);
			context.Rsp+=8;
		}
		//模拟栈空间归还
		void do_unalloc(CONTEXT & context, uint64_t size)
		{
			context.Rsp += size;
		}
		//获得指定寄存器的引用
		uint64_t & get_reg_in_context(CONTEXT & context, uint64_t reg_index)
		{
			return *(&context.Rax + reg_index);
		}
		//特化
		const uint64_t & get_reg_in_context(const CONTEXT & context, uint64_t reg_index)
		{
			//转发
			return get_reg_in_context(const_cast<CONTEXT &>(context), reg_index);
		}

#pragma endregion
#pragma region 大过程
		//试着unwind版本一的epilog
		bool try_unwind_epilog_v1(CONTEXT & context,
			utils::windows::process::ModuleInfoManager::s_module & module, const RUNTIME_FUNCTION & one)
		{
			auto unwind_info = module.m_pe_viewer->get_unwind_info(one);

			using namespace utils::windows::undocumented::unwind;

			//简单检查
			utils_assert(unwind_info->Version == 1);
			utils_assert(module.m_base + one.BeginAddress <= context.Rip);

			uint64_t rip_to_RtFuncHead = context.Rip - (module.m_base + one.BeginAddress);

			//检查是否在prolog里，不遍历链
			if (rip_to_RtFuncHead <= unwind_info->SizeOfProlog)
				return false;

			//通过尝试执行代码，检查是否在epilog里，不遍历链

			//复制一份context
			auto _context = context;

			//todo 确定没有popfq，并且栈是一次（而不是两次）恢复的

			//不检查"add rsp, x"
			//不检查"lea rsp, ..."
			//不检查"popfq"

			//检查"pop x" 和 "retn"
			bool is_meet_epilog{};
			while(1)
			{
				//简单检查
				utils_assert(_context.Rip >= module.m_base + one.BeginAddress);
				utils_assert(_context.Rip < module.m_base + one.EndAddress);

				uint16_t opcode_2B{};
				read_bytes(opcode_2B, _context.Rip);
							
				//检查pop指令
				if(uint64_t reg{};try_decode_pop_opcode(opcode_2B, reg))
				{
					do_pop(_context, reg);
					is_meet_epilog = true;
					continue;
				}
				//检查"retn"
				if ( (opcode_2B & 0xff) == 0xc3 )
				{
					do_ret(_context);
					is_meet_epilog = true;

					//使用新的context
					context = _context;
					return true;
				}
				//检查"popfq"
				if ( (opcode_2B & 0xff) == 0x9d )
				{
					utils_throw();
				}

				//简单检查
				utils_assert(!is_meet_epilog);

				//不在epilog中
				return false;
			}
		}
		//试着unwind版本二的epilog
		bool try_unwind_epilog_v2(CONTEXT & context,
			utils::windows::process::ModuleInfoManager::s_module & module, const RUNTIME_FUNCTION & one)
		{
			auto unwind_info = module.m_pe_viewer->get_unwind_info(one);

			using namespace utils::windows::undocumented::unwind;

			//简单检查
			utils_assert(unwind_info->Version == 2);
			utils_assert(module.m_base + one.BeginAddress <= context.Rip);

			uint64_t rip_to_RtFuncHead = context.Rip - (module.m_base + one.BeginAddress);

			//简单检查
			utils_assert(module.m_base + one.EndAddress > context.Rip);

			//检查是否在某个epilog里，不遍历链
			uint64_t rip_to_RtFuncTail = module.m_base + one.EndAddress - context.Rip;
			uint64_t epilog_size = utils::windows::unwind::get_epilog_size(*unwind_info);
			uint64_t i = utils::windows::unwind::get_first_EpilogPos_index(*unwind_info);
			uint64_t epilog_offset{};
			for(; i < unwind_info->CountOfCodes; i++)
			{
				if(unwind_info->UnwindCode[i].UnwindOp != UWOP_EPILOG)
					break;

				//跳过最后一个用做对齐的
				if (unwind_info->UnwindCode[i].OpInfo == 0 &&
					unwind_info->UnwindCode[i].CodeOffset == 0)
					break;

				epilog_offset = utils::windows::unwind::get_epilog_offset(unwind_info->UnwindCode[i], i);

				//简单检查
				utils_assert(epilog_offset >= epilog_size);
				utils_assert(epilog_size > 0);
				utils_assert(epilog_offset > 0);

				//在epilog的前面, 不在里面
				if(rip_to_RtFuncTail > epilog_offset)
					continue;

				//在epilog的后面, 不在里面
				if(rip_to_RtFuncTail <= epilog_offset - epilog_size)
					continue;
							
				//在这个epilog里
				break;
			}

			if(!epilog_offset)
			{
				//已检查所有的epilog，均不在其中
				return false;
			}

			//执行所有的pop，遍历链

			//沿着链，查找第一个pop操作
			i = 0;
			const UNWIND_INFO* p_unwind_info{};
			utils::windows::unwind::for_each_unwind_code_in_chain(
				*unwind_info, *module.m_pe_viewer,
				[&i, &p_unwind_info](uint64_t _i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
			{
				//todo: 确认这里，是否需要管UWOP_PUSH_MACHFRAME。因为ntdll.dll似乎是
				//      也再查找UWOP_PUSH_MACHFRAME
							
				if(unwind_code.UnwindOp == UWOP_PUSH_NONVOL)
				{
					is_stop = true;
								
					//简单检查
					utils_assert(!(unwind_info.Flags & UNW_FLAG_CHAININFO));

					i = _i;
					p_unwind_info = &unwind_info;
				}
			});

			//简单检查
			utils_assert(p_unwind_info);
			utils_assert(rip_to_RtFuncTail < epilog_offset);

			//执行所有pop操作
			uint64_t epilog_pos = module.m_base + one.EndAddress - epilog_offset;
			uint64_t virtual_pc_in_epilog{};
			uint64_t real_pc_in_epilog = epilog_offset - rip_to_RtFuncTail;
			utils::windows::unwind::for_each_unwind_code(*p_unwind_info, i,
				[&](uint64_t _i, const UNWIND_CODE & unwind_code, bool & is_stop)
			{
				if(unwind_code.UnwindOp != UWOP_PUSH_NONVOL)
				{
					is_stop = true; i = _i; return;
				}

				//验证当前位置的指令
				{
					//指令
					uint16_t opcode_2B{};
					read_bytes(opcode_2B, virtual_pc_in_epilog + epilog_pos);
					uint64_t reg{};
					utils_assert(try_decode_pop_opcode(opcode_2B, reg));
								
					//unwind_code
					uint64_t _reg = unwind_code.OpInfo;

					//验证
					utils_assert(reg == _reg);
				}

				if(virtual_pc_in_epilog >= real_pc_in_epilog)
				{
					//执行unwind_code
					uint64_t reg = unwind_code.OpInfo;
					do_pop(context, reg);
				}

				virtual_pc_in_epilog += (unwind_code.OpInfo < 8 ? 1 : 2);
			});

			//简单检查
			utils_assert(i == p_unwind_info->CountOfCodes);
						
			//验证当前位置的指令: "ret"
			uint8_t opcode_1B{};
			read_bytes(opcode_1B, virtual_pc_in_epilog + epilog_pos);
			utils_assert(opcode_1B == 0xc3);

			//执行"ret"
			do_ret(context);
			return true;

		}
		//unwind prolog或者body，版本一和版本二通用
		void unwind_prolog_or_body_v1_v2(CONTEXT & context,
			utils::windows::process::ModuleInfoManager::s_module & module, const RUNTIME_FUNCTION & one)
		{
			auto unwind_info = module.m_pe_viewer->get_unwind_info(one);

			//简单检查
			utils_assert(unwind_info->Version == 1 || unwind_info->Version == 2);

			const auto pc_in_func = context.Rip - (module.m_base + one.BeginAddress);

			/*
				因为有的dll，会在创建帧指针之前，进行save操作，比如, 在
				C:\Windows\System32\Chakra.dll中，有如下展开信息：
					.rdata:00000001806C0B48 01 16 08 05                   stru_1806C0B48  UNWIND_INFO <1, 16h, 8, 5>
					.rdata:00000001806C0B48                                                                       ; DATA XREF: .pdata:000000018072F6B4↓o
					.rdata:00000001806C0B4C 16 53                                         UNWIND_CODE <16h, 53h>  ; UWOP_SET_FPREG
					.rdata:00000001806C0B4E 13 74                                         UNWIND_CODE <13h, 74h>  ; UWOP_SAVE_NONVOL
					.rdata:00000001806C0B50 06 00                                         dw 6
					.rdata:00000001806C0B52 0E 64                                         UNWIND_CODE <0Eh, 64h>  ; UWOP_SAVE_NONVOL
					.rdata:00000001806C0B54 05 00                                         dw 5
					.rdata:00000001806C0B56 09 54                                         UNWIND_CODE <9, 54h>    ; UWOP_SAVE_NONVOL
					.rdata:00000001806C0B58 04 00                                         dw 4
					.rdata:00000001806C0B5A 04 62                                         UNWIND_CODE <4, 62h>    ; UWOP_ALLOC_SMALL
					.rdata:00000001806C0B5C 00 00 00 00                                   align 20h

					.text:00000001802ABB60 48 83 EC 38                                   sub     rsp, 38h
					.text:00000001802ABB64 48 89 6C 24 20                                mov     [rsp+var_s20], rbp
					.text:00000001802ABB69 48 89 74 24 28                                mov     [rsp+var_s28], rsi
					.text:00000001802ABB6E 48 89 7C 24 30                                mov     [rsp+var_s30], rdi
					.text:00000001802ABB73 48 8B EC                                      mov     rbp, rsp
					.text:00000001802ABB76 41 8B C1                                      mov     eax, r9d
					.text:00000001802ABB79 FF C0                                         inc     eax
					.text:00000001802ABB7B FF C0                                         inc     eax

				所以，为了在rbp还没被赋值时（1802ABB73之前），能unwind，改为如下流程：
					1.如果使用了帧指针，则，在rbp赋值之前，使用rsp，在rbp赋值之后，使用帧指针。
					2.如果没使用，使用rsp
			*/
			bool is_skiped_frame_register{};

			utils::windows::unwind::for_each_unwind_code_in_chain(*unwind_info, *module.m_pe_viewer,
				[&](uint64_t i, uint64_t j, const UNWIND_CODE & unwind_code, const UNWIND_INFO & unwind_info, bool & is_stop)
			{
				//跳过UWOP_EPILOG, 不让它影响下边的逻辑
				if(unwind_code.UnwindOp == UWOP_EPILOG) return;

				//跳过那些没执行到的
				if (j == 0 //只有第一个节点做跳过检查，其他节点全部执行
					&& unwind_code.CodeOffset >= pc_in_func)
				{
					//如果在prolog中，但还没有执行到帧指针的赋值, 则做标记
					//详情参见is_skiped_frame_register的注释
					if (unwind_code.UnwindOp == UWOP_SET_FPREG)
						is_skiped_frame_register = true;

					return;
				}

				switch(unwind_code.UnwindOp)
				{
				case UWOP_PUSH_NONVOL:
					do_pop(context, unwind_code.OpInfo);
					break;
				case UWOP_ALLOC_LARGE:
				{
					if(unwind_code.OpInfo == 0)
					{
						uint64_t size = unwind_info.UnwindCode[i+1].FrameOffset;
						do_unalloc(context, size*8);
					}
					else
					{
						//简单检查
						utils_assert(unwind_code.OpInfo == 1);
						
						uint64_t size = *(uint32_t*)&unwind_info.UnwindCode[i+1].FrameOffset;
						do_unalloc(context, size);
					}
					break;
				}
				case UWOP_ALLOC_SMALL:
				{
					uint64_t size = unwind_code.OpInfo;
					do_unalloc(context, size*8+8);
					break;
				}
				case UWOP_SET_FPREG:
				{
					//简单检查
					utils_assert(unwind_info.FrameRegister);

					//有的rsp，在prolog执行之后，还会开辟空间，所以需要把rsp，按UWOP_SET_FPREG的描述，移动回来
					
					//简单检查
					utils_assert(!is_skiped_frame_register);

					auto rsp = get_save_base_addr(unwind_info, context, false);

					//简单检查
					utils_assert(context.Rsp <= rsp);

					context.Rsp = rsp;
					break;
				}
				case UWOP_SAVE_NONVOL:
				{
					uint64_t base = get_save_base_addr(unwind_info, context, is_skiped_frame_register);
					uint64_t offset = unwind_info.UnwindCode[i+1].FrameOffset;
					uint64_t value{};
					read_bytes(value, base + offset * 8);
					get_reg_in_context(context, unwind_code.OpInfo) = value;
					break;
				}
				case UWOP_SAVE_NONVOL_FAR:
				{
					uint64_t base = get_save_base_addr(unwind_info, context, is_skiped_frame_register);
					uint64_t offset = *(uint32_t*)&unwind_info.UnwindCode[i+1].FrameOffset;
					uint64_t value{};
					read_bytes(value, base + offset);
					get_reg_in_context(context, unwind_code.OpInfo) = value;
					break;
				}
				case UWOP_EPILOG:
				{
					break;
				}
				case UWOP_SAVE_XMM128:
				{
					//todo 看看这里怎么写
					break;
				}
				case UWOP_SAVE_XMM128_FAR:
				{
					//todo 看看这里怎么写
					break;
				}
				case UWOP_PUSH_MACHFRAME:
				{
					if (unwind_code.OpInfo == 1)
						context.Rsp += 8;
					
					read_bytes(context.Rip, context.Rsp);
					read_bytes(context.Rsp, context.Rsp + 0x18);
					break;
				}
				default:
					utils_throw();
				}
			});

			do_ret(context);
		}
#pragma endregion
	};
#pragma endregion

}