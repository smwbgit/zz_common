#pragma once

#include <fstream>
#include <string>
#include <utils/inner/writer.hpp>
#include <utils/path.hpp>
#include <utils/path/real/get_information.hpp>
#include <utils/path/text/combine.hpp>

#include <utils/compiler/windows_header/maxmin__eliminate.inc>

#ifdef utils_check_status
#error
#endif
#define utils_check_status() \
do { \
	if (false == m_ofs.good()) \
		utils_throw(); \
}while(0) \



namespace utils::inner::writer::file
{
	class FileWriterBase
	{
	public:
		enum class Mode
		{
			normal,
			append,
		};
	public:
		FileWriterBase(const std::filesystem::path & filename)
		{
			init(filename, Mode::normal);
		}
		//特化
		FileWriterBase(Mode mode, const std::filesystem::path & filename)
		{
			init(filename, mode);
		}
		~FileWriterBase()
		{
			m_ofs.flush();
		}
#if 0
		template<typename T>
		void write(const T & value)
		{
			static_assert(utils::is_arithmetic_v<T> || std::is_class<T>::value, "");
			m_ofs.write((char*)&value, sizeof(T));
			utils_check_status();
		}
#endif
		void write(_In_ const void* data, _In_ uint64_t size)
		{
			m_ofs.write((char*)data, size);
			utils_check_status();
		}
		void flush()
		{
			m_ofs.flush();
			utils_check_status();
		}
		//获得当前写入位置
		void tellp(_Out_ uint64_t & pos)
		{
			auto _pos = m_ofs.tellp();
			utils_assert(_pos != std::ofstream::pos_type(-1));
//			if (_pos < 0)
//				utils_throw();
			pos = _pos;
		}
	protected:
		void init(const std::filesystem::path & filename, Mode mode)
		{
			switch(mode){
			case Mode::normal:  m_ofs.open(filename, std::ios::out | std::ios::binary);break;
			case Mode::append:{
				if(utils::path::real::get_information::single::is_exist::is_file_exist(filename.string())) m_ofs.open(filename, std::ios::in | std::ios::binary | std::ios::ate);
				else m_ofs.open(filename, std::ios::out | std::ios::binary);
				break;
			}
			default: utils_throw();}

			utils_assert(m_ofs.is_open());
		}
	private:
		std::ofstream m_ofs;
	};

	//提供一些常用函数
	template<typename Class_T>
	class FileWriterWithCommonFunctions : public Class_T
	{
	public:
		using Class_T::Class_T;

		//打开文件准备写入
		//dirname: 所在目录     e.g.  c:/abc   or  c:/abc/   or ./abc  or ./abc/
		//filename: 文件名		e.g.  1.txt
		FileWriterWithCommonFunctions(const std::u8string & dirname, const std::u8string & filename)
		{
			std::u8string _filename;
			utils::path::text::combine(_filename, dirname, filename);
			this->init(_filename);
		}
	};


//	using FileWriter = FileWriterWithSerializationSupport<FileWriterWithCommonFunctions<FileWriterBase>>;
	using FileWriter = FileWriterWithCommonFunctions<utils::inner::writer::WithCommonFunctions<FileWriterBase>>;
}
namespace utils
{
	using FileWriter = utils::inner::writer::file::FileWriter;
}

#undef utils_check_status



#include "utils/compiler/windows_header/maxmin__restore.inc"
