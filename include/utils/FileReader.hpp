#pragma once

//#include <functional>
//#include <unordered_map>
//#include <memory>
#include <fstream>
#include <filesystem>
#include "utils/inner/reader.hpp"

#include "utils/compiler/windows_header/maxmin__eliminate.inc"


#ifdef utils_check_status
#error
#endif
#define utils_check_status() \
do { \
	if (false == m_ifs.good()) \
		utils_throw(); \
}while(0) \


namespace utils::inner::reader::file
{
	class FileReaderBase
	{
	public:
		//打开文件
		FileReaderBase(const std::filesystem::path & filename)
		{
			init(filename, false);
		}
		//特化
//		FileReaderBase(const std::string & filename)
//		{
//			init(filename, false);
//		}
		void read(void* data, uint64_t size)
		{
			if (size == 0)
				return;
			if (size > (uint64_t)std::numeric_limits<std::streamsize>::max())	//TODO 看看m_ifs.read() 最多支持多大读取
				utils_throw();
			m_ifs.read((char*)data, size);
			utils_check_status();
			tellg(m_cur_pos);	//only for debug
		}
		//获得文件当前位置
		void tellg(uint64_t & pos)
		{
			auto _pos = m_ifs.tellg();
			if (_pos < 0)
				utils_throw();
			pos = _pos;
		}
		void seekg(_In_ uint64_t pos)
		{
			m_ifs.seekg(pos);
			utils_check_status();
			tellg(m_cur_pos);	//only for debug
		}
		//获得文件大小
		void get_pool_size(uint64_t & pool_size)
		{
			if (false == m_is_file_size_valid)
			{
				_get_pool_size(m_file_size);
				m_is_file_size_valid = true;
			}

			pool_size = m_file_size;
		}
		const std::ifstream & get_ifs() const
		{
			return m_ifs;
		}
		//返回try_open()是否成功
		operator bool() const
		{
			return m_is_open_success;
		}

	protected:
		struct inner__try_open
		{
		};
		//打开文件
		FileReaderBase(const std::string & filename, inner__try_open not_used)
		{
			init(filename, true);
		}
	private:
		void init(_In_ const std::filesystem::path & filename, _In_ const bool is_try_open)
		{
			m_ifs.open(filename, std::ios::in | std::ios::binary);
			if (false == m_ifs.is_open())	//打开失败
				utils_assert(is_try_open);
			m_is_open_success = true;
		}
		//获得文件大小
		void _get_pool_size(uint64_t & file_size)
		{
			uint64_t pos{};
			tellg(pos);
			m_ifs.seekg(0, std::ios_base::end);
			utils_check_status();
			tellg(file_size);
			seekg(pos);
		}

		bool m_is_file_size_valid = false;	//由于获取文件大小时会来回移动文件指针，所以缓存起来，加快速度
		uint64_t m_file_size{};
		std::ifstream m_ifs;
		uint64_t m_cur_pos{};	//only for debug
		bool m_is_open_success = false;	//是否打开成功
	};

	//提供一些常用函数
	template<typename Class_T>
	class WithCommonFunctions : public Class_T
	{
	public:
		using Class_T::Class_T;
		using Class_T::read;

		//试图打开一个文件
		static WithCommonFunctions try_open(_In_ const std::string & filename)
		{
			WithCommonFunctions input{filename, typename Class_T::inner__try_open{}};
			return std::move(input);
		}
		
		//获得文件大小
		void get_file_size(uint64_t & file_size)
		{
			this->get_pool_size(file_size);
		}

		void read_part_of_file(std::vector<uint8_t> & content, uint64_t pos, uint64_t size)
		{
			this->read_part_of_pool(content, pos, size);
		}
		void read_part_of_file(utils::data & content, uint64_t pos, uint64_t size)
		{
			this->read_part_of_pool(content, pos, size);
		}
		void read_whole_file(std::vector<uint8_t> & content)
		{
			this->read_whole_pool(content);
		}
		void read_whole_file(std::string & content)
		{
			this->read_all(content);
		}

	};

	using FileReader = WithCommonFunctions<utils::inner::reader::WithCommonFunctions<FileReaderBase>>;
}
namespace utils
{
	using FileReader = utils::inner::reader::file::FileReader;
}


#undef utils_check_status



#include "utils/compiler/windows_header/maxmin__restore.inc"
