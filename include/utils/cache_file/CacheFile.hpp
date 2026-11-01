#pragma once

#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>

namespace utils::cache_file
{
	//缓存文件
	class CacheFile
	{
	private:
		static const std::u8string m_magic;
	private:
		std::u8string m_filename;
		std::optional<utils::data> m_signature;
	public:
		CacheFile(const std::u8string & filename)
			: m_filename{ filename }
		{
		}
	public:
		//设置签名
		void set_signature(utils::data_view signature)
		{
			//简单检查
			utils_assert(!m_signature);

			//保存
			m_signature = utils::data{signature};
		}
		//获取内容
		std::optional<utils::data> get_payload()
		{
			//不存在缓存文件
			if(!utils::real_path::is_file_exist(m_filename))
			{
				//返回空
				return {};
			}

			//打开缓存文件
			utils::FileReader m_input{m_filename};

			//读取magic
			//todo: 看看怎么把读取中，出现的问题，捕获住，然后返回空
			std::u8string magic;
			m_input.serializer & magic;

			//magic不正确
			if(m_magic != magic)
			{
				//返回空
				return {};
			}

			//读取签名
			utils::data signature;
			m_input.serializer & signature;
			
			//简单检查
			utils_assert(m_signature);

			//签名不正确
			if(*m_signature != signature)
			{
				//返回空
				return {};
			}

			//读取内容
			utils::data payload;
			m_input.serializer & payload;

			//给出
			return payload;
		}
		//生成缓存文件
		void save(utils::data_view payload)
		{
			//打开缓存文件
			utils::FileWriter m_output{m_filename};

			//写入magic
			m_output.serializer & m_magic;
			
			//简单检查
			utils_assert(m_signature);

			//写入签名
			m_output.serializer & *m_signature;

			//写入内容
			m_output.serializer & payload;
		}
	};

	inline const std::u8string CacheFile::m_magic = u8"utils::cache_file::CacheFile 20251202";
}