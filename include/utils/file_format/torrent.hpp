#pragma once

#include <utils/data_encoding/bencode.hpp>
#include <utils/data_encoding/sha1.hpp>
#include <utils/FileReader.hpp>
#include <utils/vector.hpp>

//todo：目前版本，仅针对torrent文件的第一个版本。还不能解析，v2版本的torrent文件。
namespace utils::file_format::torrent
{
	//解析一个torrent文件
	class FileReader
	{
	private:
		utils::data m_data;
		std::unique_ptr<utils::data_encoding::bencode::Decoder> m_bencode_decoder;
	public:
		FileReader(const std::u8string & filename)
		{
			//打开文件
			utils::FileReader m_input{filename};

			//读取文件
			m_input.read_all(m_data);

			//解析全部内容
			m_bencode_decoder = std::make_unique<utils::data_encoding::bencode::Decoder>(m_data);
		}
		//获得服务器列表。第一个为首选服务器
		//note: 此函数主要是为了内部调试时使用。因为此列表被压瘪了，不再有tier区分。
		std::vector<std::u8string> get_announce_list() const
		{
			//获取文件内容
			const auto & tree = m_bencode_decoder->get_tree();

			//找到相应的节点
			const auto & root = tree.m_root_node->get_dictionary();
			const auto & announce = utils::map(root).get_existed_value(u8"announce");
			const auto & announce_list = utils::map(root).get_existed_value(u8"announce-list");
			const auto & _announce_list = announce_list->get_list();

			//准备数据
			std::vector<std::u8string> list;

			//添加数据
			list.push_back(announce->get_string());

			//添加数据
			for(const auto & tier : _announce_list)
			{
				const auto & _tier = tier->get_list();

				for(const auto & one : _tier)
				{
					list.push_back(one->get_string());
				}
			}

			//去重
			utils::vector(list).remove_duplicate();

			//给出
			return list;
		}
		//获取info字段的hash值
		utils::data get_info_hash() const
		{
			//获取文件内容
			const auto & tree = m_bencode_decoder->get_tree();

			//找到相应的节点
			const auto & root = tree.m_root_node->get_dictionary();
			const auto & info = utils::map(root).get_existed_value(u8"info");

			//获取info字段的所有字节
			utils::data content = m_data.subdata(info->m_offset, info->m_size);

			//计算sha1
			const auto sha1 = utils::data_encoding::sha1::encode(content);

			//给出
			return sha1;
		}
	};
}