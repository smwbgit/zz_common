#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows
{
	//共享内存
	//note: 如果是初次创建，则会在构造函数返回前，完成T的初始化; 否则会在构造函数返回前，完成对初始化的检查
	template<typename T>
	class SharedMemory
	{
	private:
		struct s_data
		{
			bool is_inited{};
			T value{};
		};

	private:
		HANDLE m_file{};
		s_data* m_data{};
		bool m_is_already_exist{};

	public:
		SharedMemory(const std::string & name)
		{
			m_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(s_data), name.c_str()); utils_assert(m_file);

			//检查是否为首次创建
			auto error = GetLastError(); utils_assert(error == 0 || error == ERROR_ALREADY_EXISTS);
			m_is_already_exist = error == ERROR_ALREADY_EXISTS;

			m_data = (s_data*)MapViewOfFile(m_file, FILE_MAP_ALL_ACCESS, 0, 0, 0); utils_assert(m_data);

			if (m_is_already_exist)
			{
				//等待初始化完成

				while (!m_data->is_inited)
					Sleep(1);
			}
			else
			{
				//初始化

				new (m_data) s_data;
				m_data->is_inited = true;
			}
		}
		~SharedMemory()
		{
			utils_assert_abort(UnmapViewOfFile(m_data));
			utils_assert_abort(CloseHandle(m_file));
		}
		T* operator->()
		{
			return &m_data->value;
		}
		bool is_already_exist() const
		{
			return m_is_already_exist;
		}
	};
}