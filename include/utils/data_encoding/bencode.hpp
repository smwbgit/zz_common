#pragma once

#include <utils/DataReader.hpp>
#include <utils/map.hpp>
#include <utils/text/number.hpp>

namespace utils::data_encoding::bencode
{
	namespace inner
	{
		//一个节点
		class Node_; using Node = std::unique_ptr<Node_>;
		class Node_
		{
		public:
			//节点类型
			enum class NodeType
			{
				String,
				Number,
				List,
				Dictionary,
			};
		public:
			//节点类型
			std::optional<NodeType> m_type;

			//依照m_type的当前值，使用以下四个中的一个
			std::u8string m_string{};
			utils::s8 m_number{};
			std::vector<Node> m_list;
			std::map<std::u8string, Node> m_dictionary;

			//此节点，在原始数据中的偏移和大小
			utils::u8 m_offset{};
			utils::u8 m_size{};

		public:
			const std::u8string & get_string() const
			{
				//简单检查
				utils_assert(is_string());

				//给出
				return m_string;
			}
			const std::map<std::u8string, Node> & get_dictionary() const
			{
				//简单检查
				utils_assert(is_dictionary());

				//给出
				return m_dictionary;
			}
			const std::vector<Node> & get_list() const
			{
				//简单检查
				utils_assert(is_list());

				//给出
				return m_list;
			}
			utils::s8 get_number() const
			{
				//简单检查
				utils_assert(is_number());

				//给出
				return m_number;
			}
			bool is_dictionary() const
			{
				return get_type() == NodeType::Dictionary;
			}
			bool is_string() const
			{
				return get_type() == NodeType::String;
			}
			bool is_list() const
			{
				return get_type() == NodeType::List;
			}
			bool is_number() const
			{
				return get_type() == NodeType::Number;
			}
			NodeType get_type() const
			{
				//简单检查
				utils_assert(m_type);

				return m_type.value();
			}
		};

		//整个内容
		class Tree
		{
		public:
			//根节点
			Node m_root_node;
		};
	}

	//解码
	class Decoder
	{
	private:
		utils::DataReader m_input;
		inner::Tree m_tree;
	public:
		Decoder(utils::data_view data)
			: m_input{data}
		{
			//读取根节点
			m_tree.m_root_node = read_one_node();

			//简单检查
			m_input.readed_all();
		}
		//获得解码后的内容
		const inner::Tree & get_tree() const
		{
			return m_tree;
		}
	private:
		//读取一个节点
		inner::Node read_one_node()
		{
			using NodeType = inner::Node_::NodeType;

			//简单检查
			utils_assert(!m_input.is_readed_all());

			//检查开头字符
			char c{};
			m_input.peek(c);

			//准备数据
			inner::Node node = std::make_unique<inner::Node_>();

			//获取当前读取位置，保存为此节点，的开始位置
			u8 begin_position{};
			m_input.tellg(begin_position);

			//数字
			if (c == 'i')
			{
				//跳过
				m_input.read_this(c);

				//读取
				const auto number = read_one_number('e');

				//保存
				node->m_type = NodeType::Number;
				node->m_number = number;
			}
			//列表
			else if (c == 'l')
			{
				//跳过
				m_input.read_this(c);

				//设置类型
				node->m_type = NodeType::List;

				//对于每个子节点
				while(1)
				{
					//检查开头字符
					char c{};
					m_input.peek(c);

					//列表结束
					if(c == 'e')
					{
						//跳过
						m_input.read_this(c);

						break;
					}

					//读取一个节点
					auto child_node = read_one_node();

					//保存
					node->m_list.push_back(std::move(child_node));
				}
			}
			//字典
			else if (c == 'd')
			{
				//跳过
				m_input.read_this(c);

				//设置类型
				node->m_type = NodeType::Dictionary;

				//对于每组子节点
				while(1)
				{
					//检查开头字符
					char c{};
					m_input.peek(c);

					//字典结束
					if(c == 'e')
					{
						//跳过
						m_input.read_this(c);

						break;
					}

					//读取一个节点，做为key
					auto key_node = read_one_node();

					//获取key字符串
					const auto key_string = key_node->get_string();

					//读取一个节点，做为value
					auto value_node = read_one_node();

					//保存
					utils::map(node->m_dictionary).insert_not_exist(key_string, std::move(value_node));
				}
			}
			//字符串
			else
			{
				//读取长度
				const auto number = read_one_number(':');

				//简单检查
				utils_assert(number >= 0);

				//设置类型
				node->m_type = NodeType::String;

				//读取字符串
				m_input.text_u8.read(node->m_string, number);
			}

			//获取当前读取位置，保存为此节点，的结束位置
			u8 end_position{};
			m_input.tellg(end_position);

			//简单检查
			utils_assert(end_position > begin_position);

			//保存
			node->m_offset = begin_position;
			node->m_size = end_position - begin_position;

			//给出
			return std::move(node);
		}
		//读取一个数字
		utils::s8 read_one_number(char end_char)
		{
			//读取
			std::u8string text;
			m_input.text_u8.read_until(text, end_char, false);

			//转换
			utils::s8 number{};
			utils::text::number::from_text(number, text);

			//给出
			return number;
		}
	};
}