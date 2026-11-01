#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows::process::attribute_list
{
	//属性列表
	class AttributeList
	{
	public:
		LPPROC_THREAD_ATTRIBUTE_LIST m_list{};
	private:
		utils::data m_list_buffer;
		GROUP_AFFINITY m_group_affinity{};	//因api要求，所有设置的值，都要在列表使用期间存在，所以，放这里。
	public:
		AttributeList(DWORD max_count)
		{
			//简单检查
			utils_assert(max_count >= 1);
			utils_assert(max_count < 200);	//此条中的200为随意指定的值（因所有属性的总数似乎不到200）。仅确保它不在明显错误的范围中。

			//计算所需大小
			SIZE_T size{};
			utils_assert(FALSE == InitializeProcThreadAttributeList(NULL, max_count, 0, &size));
			utils_assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

			//分配内存
			m_list_buffer.resize(size);

			//初始化
			SIZE_T size_copy = size;
			utils_assert(InitializeProcThreadAttributeList((LPPROC_THREAD_ATTRIBUTE_LIST)m_list_buffer.addr(), max_count, 0, &size));

			//简单检查
			utils_assert(size_copy == size);

			//保存
			m_list = (LPPROC_THREAD_ATTRIBUTE_LIST)m_list_buffer.addr();
		}
		~AttributeList()
		{
			//清理
			DeleteProcThreadAttributeList(m_list);
		}
		AttributeList(const AttributeList &) = delete;	//禁止复制
		auto operator=(const AttributeList &) = delete;	//禁止复制
		AttributeList(AttributeList &&) = delete;	//todo: 让它能move
		auto operator=(AttributeList &&) = delete;	//todo: 让它能move
	public:
		//设置cpu亲和性
		//note: 此属性不能用于CreateProcess，会报 ERROR_INVALID_PARAMETER 错误
		void set_affinity_mask(const DWORD affinity_mask)
		{
			//设置亲和性
			m_group_affinity.Mask = affinity_mask;
			m_group_affinity.Group = 0;

			//更新属性
			utils_assert(UpdateProcThreadAttribute(m_list, 0,
				PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY,
				&m_group_affinity, sizeof(m_group_affinity),
				NULL, NULL));
		}
	};
}