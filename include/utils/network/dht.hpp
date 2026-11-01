#pragma once

#include <mutex>
#include <utils/data_encoding/bencode.hpp>
#include <utils/function.hpp>
#include <utils/network/ip.hpp>
#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>
#include <utils/chrono/system_clock.hpp>
#include <utils/text/filename/save_as_current_time.hpp>
#include <utils/data_encoding/crc32.hpp>
#include <utils/windows/ui/console.hpp>

namespace utils::network::dht
{
	//代表一个20字节的id
	class Id
	{
	public:
		utils::SizedData<20> m_data;

	public:
		//构造函数
		explicit Id(utils::data_view data)
			: m_data{data}
		{
		}
		//构造函数
		Id() = default;
		Id(const Id &) = default;
		Id(Id &&) = default;
	public:
		//赋值函数
		Id & operator = (utils::data_view data)
		{
			m_data = data;
			return *this;
		}
		//赋值函数
		Id & operator = (const Id &) = default;
		Id & operator = (Id &&) = default;
	public:
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
#pragma region 距离比较
		//代表一个id间的距离
		class Distance
		{
		public:
			utils::SizedData<20> m_data;
		public:
			//比较距离大小
			bool operator <(const Distance & other) const
			{
				//对于每一个字节
				for(const auto i : utils::iterator_index(m_data.size()))
				{
					//准备数据
					const auto b_1 = m_data[i];
					const auto b_2 = other.m_data[i];

					//如果相同，则比较下一字节
					if(b_1 == b_2)
						continue;

					//给出结果
					return b_1 < b_2;
				}

				//两者相同
				return false;
			}
		};
		//获取距离
		Distance get_distance(const Id & other) const
		{
			//准备数据
			Distance distance;

			//对于每一个字节
			for(const auto i : utils::iterator_index(m_data.size()))
			{
				//准备数据
				const auto b_1 = this->m_data[i];
				const auto b_2 = other.m_data[i];

				//异或
				const auto b = b_1 ^ b_2;

				//保存
				distance.m_data[i] = b;
			}

			//给出
			return distance;
		}
#pragma endregion
	};

	//代表一个节点
	template<utils::network::ip::version::Version ip_version>
	class Node
	{
	public:
		Id id;
		utils::network::ip::Endpoint<ip_version> endpoint;

	public:
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & id;
			file & endpoint;
		}
	};

	//负责生成id前缀，以及，验证id前缀。基于，BEP42安全扩展
	//note: 非线程安全
	template<utils::network::ip::version::Version ip_version>
	class IdIpBinder
	{
	private:
		using Ip = utils::network::ip::Ip<ip_version>;
	private:
		static inline const std::array<u1, 20> m_mask_id{0xff,0xff,0xf8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07};
		static inline const std::array<u1, 4> m_mask_ipv4{0x03, 0x0f, 0x3f, 0xff};
		static inline const std::array<u1, 8> m_mask_ipv6{0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
	public:
		//生成一个符合规范的id。其中自由位随机填充
		Id make_id_by_random(const Ip & ip, u1 random = 0) const
		{
			//生成一个符合规范的id
			Id id_0 = make_id(ip, random);

			//再生成一个随机id
			Id id_1; id_1.m_data.assign_by_random();

			//把它俩按掩码，揉一块
			Id id_2;
			for(u8 i = 0; i < 20; i++)
			{
				//准备数据
				const auto a = id_0.m_data[i];
				const auto b = id_1.m_data[i];
				const auto & c = id_2.m_data[i];
				const auto m = m_mask_id[i];
				const auto m_0 = m;
				const auto m_1 = ~m;

				c = (a & m_0) | (b & m_1);
			}

			//给出
			return id_2;
		}
		//生成一个符合规范的id。其中自由位为0
		//random: 算法中有3位的自由度。标准算法里，推荐是随机取这三位
		Id make_id(const Ip & ip, u1 random = 0) const
		{
#if 0	//BEP42原文示例代码
				uint8_t* ip; // 我们的外部 IPv4 或 IPv6 地址（网络字节序）
				int num_octets {}; // 要考虑的 ip 字节数（4 或 8）
				uint8_t node_id[20]; // 结果节点 ID

				uint8_t v4_mask[] = { 0x03, 0x0f, 0x3f, 0xff };
				uint8_t v6_mask[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
				uint8_t* mask = num_octets == 4 ? v4_mask : v6_mask;

				for (int i = 0; i < num_octets; ++i)
				ip[i] &= mask[i];

				uint32_t rand = std::rand() & 0xff;
				uint8_t r = rand & 0x7;
				ip[0] |= r << 5;

				uint32_t crc = 0;
				crc = crc32c(crc, ip, num_octets);

				// 只取 crc 的高 21 位
				node_id[0] = (crc >> 24) & 0xff;
				node_id[1] = (crc >> 16) & 0xff;
				node_id[2] = ((crc >> 8) & 0xf8) | (std::rand() & 0x7);
				for (int i = 3; i < 19; ++i) node_id[i] = std::rand();
				node_id[19] = rand;
#endif

			//准备数据
			Id id;

			//对ip进行"与"运算

			//准备好要用的掩码
			const auto & mask_ip = [&]() -> const auto &
			{
				if constexpr(ip_version::version == 4)
				{
					return m_mask_ipv4;
				}
				else if constexpr(ip_version::version == 6)
				{
					return m_mask_ipv6;
				}
				else
				{
					//报错
					static_assert(false);
				}
			}();

			//准备好ip数据
			auto ip_data = [&]()
			{
				if constexpr(ip_version::version == 4)
				{
					return ip.m_data;
				}
				else if constexpr(ip_version::version == 6)
				{
					//只计算前8字节
					return utils::data_view(ip.m_data).sub_data(0, 8);
				}
				else
				{
					//报错
					static_assert(false);
				}
			}();

			//简单检查
			utils_assert(mask_ip.size() <= ip_data.size());

			//进行"与"运算
			for (u8 i = 0; i < mask_ip.size(); i++)
				ip_data[i] &= mask_ip[i];

			//把随机数，加到ip的开头
			ip_data[0] |= u1(random & 0x7) << 5;

			//对ip数据进行 crc32c 运算
			const u4 crc = utils::data_encoding::
				crc32::encode(ip_data, utils::data_encoding::crc32::standard::Castagnoli);

			//只取crc的高21位
			id.m_data[0] = (crc >> 24) & 0xff;
			id.m_data[1] = (crc >> 16) & 0xff;
			id.m_data[2] = ((crc >> 8) & 0xf8) | (id.m_data[2] & 0x7);

			//再把随机数，写在最后
			id.m_data[19] = u1(random & 0x7) | (id.m_data[19] & 0xf8);

			//给出
			return id;
		}
		//判断id和ip是否符合规范
		bool is_id_ok(const Id & id, const Ip & ip) const
		{
			//获取id的最后3位，还原为随机值
			const u1 random = id.m_data[19] & 0x7;

			//生成一个符合规范的id
			Id id_2 = make_id(ip, random);

			//把它俩按掩码，检查
			for(u8 i = 0; i < 20; i++)
			{
				//准备数据
				const auto a = id.m_data[i];
				const auto b = id_2.m_data[i];
				const auto m = m_mask_id[i];

				//计算
				const auto r_1 = a & m;
				const auto r_2 = b & m;

				//出现不一致
				if (r_1 != r_2)
				{
					//给出
					return false;
				}
			}

			//给出
			return true;
		}
	};

	//管理所有节点信息
	//note: 线程安全
	template<utils::network::ip::version::Version ip_version>
	class NodeManager
	{
	private:
		using Node = Node<ip_version>;
		using Ip = utils::network::ip::Ip<ip_version>;
		using IpProviderPrefix = utils::network::ip::IpProviderPrefix<ip_version>;
	public:
		struct s_param
		{
			struct s_optional
			{
				//保存节点的目录
				std::optional<std::u8string> dirname_save;
			};
			s_optional optional;
		};
		s_param m_param;
	private:
		//代表一个节点
		struct NodeEx
		{
			Node node;

			//以下是节点的历史数据
			std::optional<utils::chrono::system_clock::time_point> last_send_time;	//最后向其发送时间
			std::optional<utils::chrono::system_clock::time_point> last_recv_time;	//最后从其接收时间
			bool is_talk_crazy{};	//是否乱发消息

		public:
			//是否为死节点
			bool is_dead_node() const
			{
				//还从没发送过，不是死节点
				if(!last_send_time)
					return false;

				//刚发送出去，还来不及测试出，是否为死节点。姑且当不是死节点处理
				if(utils::chrono::system_clock::now() - last_send_time.value() <
					std::chrono::seconds{10})
				{
					return false;
				}

				//发送后，有回音，不是死节点
				if(last_recv_time && last_recv_time.value() >= last_send_time.value())
					return false;

				//是死节点
				return true;
			}
			//序列化
			template<bool is_saving, typename serializer_T>
			void serialize_support(serializer_T & file)
			{
				file & node;
				file & last_send_time;
				file & last_recv_time;
				file & is_talk_crazy;
			}
		};
	private:
		//锁
		mutable std::mutex m_mutex;
		//所有节点
		//note:此版本的代码里，一个ip只能有一个节点
		std::map<IpProviderPrefix, NodeEx> m_all_node;
	public:
		//获取所有节点
		//is_keep_dead: 是否给出死节点（死节点：向其发送信息，但不回）
		//is_keep_crazy: 是否给出乱发消息的节点
		std::vector<Node> get_all_node(bool is_keep_dead = false, bool is_keep_crazy = false) const
		{
			//获取锁
			std::lock_guard lock_guard{m_mutex};

			//准备数据
			std::vector<Node> all_node;

			//对于每个节点
			for(const auto & node : m_all_node)
			{
				//不要死节点
				if(!is_keep_dead)
				{
					//是死节点
					if(node.second.is_dead_node())
					{
						//跳过
						continue;
					}
				}

				//不要乱语节点
				if(!is_keep_crazy)
				{
					//是乱语节点
					if(node.second.is_talk_crazy)
					{
						//跳过
						continue;
					}
				}

				//复制
				all_node.push_back(node.second.node);
			}

			//给出
			return all_node;
		}
		//添加一个节点
		//is_check_ip_bind: 是否检查id和ip符合前缀一致规范
		void add(const Node & node, bool is_check_ip_bind = true)
		{
			//获取锁
			std::lock_guard lock_guard{m_mutex};

			//获取ip的服务商前缀
			const auto provider_prefix = node.endpoint.ip.get_provider_prefix();

			//已有此ip
			if(utils::map(m_all_node).is_exist(provider_prefix))
			{
				//显示提示
				//const auto console_text = utils::text::format::format(u8"existed ip:%d\n", remote_ip.c_str(), remote_port);
				//utils::windows::ui::console::write_text(console_text);

				//更新统计

				//不再继续
				return;
			}

			//要求验证id前缀一致性
			if(is_check_ip_bind)
			{
				//验证一下，对方的id和其ip是否符合规范

				//准备数据
				utils::network::dht::IdIpBinder<ip_version> id_ip_binder;

				//不符合规范
				if(!id_ip_binder.is_id_ok(node.id, node.endpoint.ip))
				{
					//显示提示
					utils::windows::ui::console::write_text(u8"id not ok\n");

					//更新统计

					//不再继续
					//return;
				}
				else
				{
					//显示提示
					utils::windows::ui::console::write_text(u8"id ok\n");
				}
			}

			//创建一个新的节点
			auto & _node = m_all_node[provider_prefix];

			//保存
			_node.node = node;
		}
		//更新节点的最后接收时间
		void update_recv_time(const Ip & ip)
		{
			update_something_if_exist(ip, [this](NodeEx & node)
			{
				//更新信息
				node.last_recv_time = utils::chrono::system_clock::now();
			});
		}
		//更新节点的最后发送时间
		void update_send_time(const Ip & ip)
		{
			update_something_if_exist(ip, [this](NodeEx & node)
			{
				//更新信息
				node.last_send_time = utils::chrono::system_clock::now();
			}, true);
		}
		//标记一个节点，乱发数据
		void flag_talk_crazy(const Ip & ip)
		{
			update_something_if_exist(ip, [this](NodeEx & node)
			{
				//更新信息
				node.is_talk_crazy = true;
			});
		}
		//保存数据
		void save() const
		{
			//获取锁
			std::lock_guard lock_guard{m_mutex};

			//生成一个文件名
			const auto filename = [&]()
			{
				//所在目录
				const auto dirname = get_dirname_save();

				//生成文件名
				const auto filename = utils::text::filename::
					save_as_current_time::make_by_dirname(dirname, u8".dht_nodes");

				//给出
				return filename;
			}();

			//打开文件
			utils::FileWriter output{filename};

			//写入数据
			output.serializer & m_all_node;
		}
		//载入数据
		//return: 如果找到载入文件，返回true。否则返回false
		bool load()
		{
			//获取锁
			std::lock_guard lock_guard{m_mutex};

			//简单检查
			utils_assert(m_all_node.empty());

			//获取最新文件的文件名
			const auto filename = [&]()
			{
				//所在目录
				const auto dirname = get_dirname_save();

				//获取文件名
				const auto filename = utils::path::real::get_information::global::
					get_last_write_file::by_directory(dirname);

				//给出
				return filename;
			}();

			//如果获取到
			if(filename)
			{
				//准备数据
				const auto & _filename = filename.value();

				//简单检查
				utils_assert(_filename.ends_with(u8".dht_nodes"));

				//打开文件
				utils::FileReader input{_filename};

				//读取数据
				input.serializer & m_all_node;

				//简单检查
				input.readed_all();

				//已载入
				return true;
			}

			//没找到可载入的文件
			return false;
		}
	private:
		//更新节点的某些信息，如果节点存在的话
		void update_something_if_exist(const Ip & ip, std::function<void(NodeEx & node)> meet_node, bool is_must_exist = false)
		{
			//准备函数：获取节点
			auto get_node = [this](const Ip & ip) -> std::optional<NodeEx*>
			{
				//获取ip的服务商前缀
				const auto provider_prefix = ip.get_provider_prefix();

				//没有此ip
				if(!utils::map(m_all_node).is_exist(provider_prefix))
					return {};

				//准备数据
				auto & node = utils::map(m_all_node).get_existed_value(provider_prefix);

				//没有此ip
				if(node.node.endpoint.ip != ip)
					return {};

				//给出
				return &node;
			};

			//获取锁
			std::lock_guard lock_guard{m_mutex};

			//获取节点
			auto node = get_node(ip);

			//要求必须存在
			if(is_must_exist)
			{
				//简单检查
				utils_assert(node);
			}

			//节点不存在
			if(!node)
			{
				//不再继续
				return;
			}

			//准备数据
			auto & _node = *node.value();

			//进行修改
			meet_node(_node);
		}
		//获取存储位置
		std::u8string get_dirname_save() const
		{
			//简单检查
			utils_assert(m_param.optional.dirname_save);

			//准备数据
			const auto & dirname = m_param.optional.dirname_save.value();

			//ip版本
			const auto & text_ip_version = ip_version::text_short;

			//存储位置
			const auto dirname_1 = std::filesystem::path{dirname} / text_ip_version;
			const auto dirname_2 = dirname_1.u8string();

			//给出
			return dirname_2;
		}
	};

	//负责，消息的拼装和解析
	//note: 拼出发送的消息，解析收到的消息
	//note: 非线程安全
	template<utils::network::ip::version::Version ip_version>
	class PacketCodec
	{
	private:
		using Node = Node<ip_version>;
	public:
		struct s_param
		{
			struct s_optional
			{
				std::function<void()> on_message_find_node;
				std::function<void(std::vector<Node> && nodes, Id && remote_id)> on_message_find_node_reply;
				std::function<void(Id && info_hash)> on_message_get_peers;
				std::function<void()> on_message_get_peers_reply;
				std::function<void()> on_message_ping;
				std::function<void()> on_message_ping_reply;
				std::function<void()> on_message_xxx;
				std::function<void()> on_message_xxx_reply;
				std::function<void()> on_message_invalid;
			};
			s_optional optional;
		};
		s_param m_param;
	public:
		//拼出find_node消息
		utils::data make_message_find_node(const Id & my_id, const Id & target_id) const
		{
			return utils::data{"d1:ad2:id20:"} + my_id.m_data +
				utils::data{"6:target20:"} + target_id.m_data +
				utils::data{"e1:q9:find_node1:t2:aa1:y1:qe"};
		}
		//解码消息
		void decode_message(utils::data_view data) const
		{
			//准备函数: 消息格式不正确
			auto on_message_invalid = [this]()
			{
				//通知用户
				utils::function(m_param.optional.on_message_invalid).call_if_exist();
			};

			//准备数据
			std::unique_ptr<utils::data_encoding::bencode::Decoder> decoder;

			//解析数据
			try
			{
				decoder = std::make_unique<utils::data_encoding::bencode::Decoder>(data);
			}
			//解析出错
			catch(utils::Exception &)
			{
				//格式不正确，不再继续
				return on_message_invalid();
			}

			//格式不正确，不再继续
			if(!decoder->get_tree().m_root_node->is_dictionary())
				return on_message_invalid();

			//定位到相应节点
			const auto & root = decoder->get_tree().m_root_node->get_dictionary();

			//note：以下注释中，之所以是估计，是因为，我不是很确定，是不是这样。

			//存在q，估计这是一个请求消息
			if(utils::map(root).is_exist(u8"q"))
			{
				//定位相应节点
				const auto & _q = utils::map(root).get_existed_value(u8"q");
				
				//格式不正确，不再继续
				if(!_q->is_string())
					return on_message_invalid();

				//定位相应节点
				const auto & q = _q->get_string();

				//这是一条get_peers的请求消息
				if(q == u8"get_peers")
				{
					//定位相应节点
					const auto & _a = utils::map(root).get_existed_value(u8"a");

					//格式不正确，不再继续
					if(!_a->is_dictionary())
						return on_message_invalid();

					//定位相应节点
					const auto & a = _a->get_dictionary();
				
					//格式不正确，不再继续
					if(!utils::map(a).is_exist(u8"info_hash"))
						return on_message_invalid();

					//定位相应节点
					const auto & _info_hash = utils::map(a).get_existed_value(u8"info_hash");
				
					//格式不正确，不再继续
					if(!_info_hash->is_string())
						return on_message_invalid();

					//定位相应节点
					const auto & info_hash = _info_hash->get_string();

					//格式不正确，不再继续
					if(info_hash.size() != 20)
						return on_message_invalid();

					//简单转换
					Id the_info_hash{info_hash};

					//通知用户
					utils::function(m_param.optional.on_message_get_peers).call_if_exist(std::move(the_info_hash));

					//处理完毕
					return;
				}
				//其他类型的请求消息
				else
				{
					//todo
					return;
				}
			}
			//存在r，估计这是一个回复消息
			else if(utils::map(root).is_exist(u8"r"))
			{
				//定位相应节点
				const auto & _r = utils::map(root).get_existed_value(u8"r");

				//格式不正确，不再继续
				if(!_r->is_dictionary())
					return on_message_invalid();

				//定位相应节点
				const auto & r = _r->get_dictionary();

				//ipv4时，只关注nodes节点。ipv6时，只关注nodes6节点。其他的处理逻辑相同，除了
				//    里边的地址一个是4字节的，一个是16字节的

				//关注的节点名
				const auto node_name = []() -> std::u8string
				{
					//ipv4
					if constexpr (ip_version::version == 4)
					{
						return u8"nodes";
					}
					//ipv6
					else if constexpr (ip_version::version == 6)
					{
						return u8"nodes6";
					}
					else
					{
						//报错
						static_assert(false);
					}
				}();

				//不存在nodes节点
				if(!utils::map(r).is_exist(node_name))
				{
					//todo
					return;
				}

				//定位相应节点
				const auto & _nodes_info = utils::map(r).get_existed_value(node_name);

				//格式不正确，不再继续
				if(!_nodes_info->is_string())
					return on_message_invalid();

				//定位相应节点
				const auto & nodes_info = _nodes_info->get_string();

				//一个节点的长度
				const auto entry_size = 20 + ip_version::ip_size + 2;

				//数据格式不标准
				if(nodes_info.length() % entry_size != 0)
				{
					//不再继续
					return on_message_invalid();
				}

				//简单检查
				utils_assert(nodes_info.length() % entry_size == 0);

				//把这些节点数据分开

				//准备数据
				utils::DataReader data_reader{{nodes_info.c_str(), nodes_info.length()}};

				//准备数据
				std::vector<Node> nodes;

				//循环读取
				while(1)
				{
					//没有数据要处理
					if(data_reader.is_readed_all())
						break;

					//读取一个节点的数据
					Node node;
					data_reader.read(node.id.m_data);
					data_reader.read(node.endpoint.ip.m_data);
					data_reader.read(node.endpoint.port.m_data);

					//大小端转换
					node.endpoint.port.change_byte_order();

					//保存
					nodes.push_back(std::move(node));
				}

				//读取id字段
				Id remote_id;
				{
					//格式不正确，不再继续
					if(!utils::map(r).is_exist(u8"id"))
						return on_message_invalid();

					//定位相应节点
					const auto & id_node_0 = utils::map(r).get_existed_value(u8"id");

					//格式不正确，不再继续
					if(!id_node_0->is_string())
						return on_message_invalid();

					//定位相应节点
					const auto & id_node_1 = id_node_0->get_string();

					//保存
					remote_id = Id{id_node_1};
				}

				//通知用户
				utils::function(m_param.optional.on_message_find_node_reply).call_if_exist(std::move(nodes), std::move(remote_id));

				//处理完毕
				return;
			}
			//估计它格式不规范
			else
			{
				//不再继续
				return on_message_invalid();
			}
		}
	};

	//负责找出离目标最近的节点
	//note: 非线程安全
	template<utils::network::ip::version::Version ip_version>
	class SearchManager
	{
	private:
		using NodeManager = NodeManager<ip_version>;
		using Node = Node<ip_version>;
		using Ip = utils::network::ip::Ip<ip_version>;
	private:
		NodeManager & m_node_manager;
		Id m_target_id;
		std::set<Ip> m_all_visited_node;
	public:
		SearchManager(NodeManager & node_manager, const Id & target_id)
			: m_node_manager{node_manager}
			, m_target_id{target_id}
		{
		}
	public:
		//获取一个离目标最近，又未访问过的节点
		//return: 如果不存在，则返回空
		std::optional<Node> get_next_node()
		{
			//获取最新的节点列表
			auto all_node = m_node_manager.get_all_node();

			//把它们，按离目标的远近，排个序，近的在前
			std::sort(all_node.begin(), all_node.end(), [this](const auto & a, const auto & b)
			{
				const auto distance_a = a.id.get_distance(m_target_id);
				const auto distance_b = b.id.get_distance(m_target_id);

				return distance_a < distance_b;
			});

			//从前面开始找，找到一个还未访问过的节点
			for(const auto & node : utils::iterator_and_give_me_i(all_node))
			{
				//这里加个限制，如果已经有足够多的，相邻节点被访问过，则更远的节点就不再访问啦
				if(node.i > 100)
					return {};

				//此节点已经访问过
				if(utils::set(m_all_visited_node).is_exist(node.value.endpoint.ip))
				{
					//继续下一个
					continue;
				}

				//此节点被选中，做为返回值。在此之前，先对此节点进行标记，表示此节点已经被访问过
				utils::set(m_all_visited_node).insert_not_exist(node.value.endpoint.ip);

				//给出
				return node.value;
			}

			//没有找到合适的节点
			return {};
		}
	};
}

#if 0	//一个遍历dht网络，不断寻找新节点的程序
#include <utils/network/dht.hpp>
#include <utils/network/udp/Udp.hpp>
#include <utils/network/ip.hpp>
#include <utils/path/real/get_information.hpp>
#include <utils/text/filename/save_as_current_time.hpp>
#include <boost/asio.hpp>
#include <utils/thread/Thread.hpp>
#include <utils/this_thread.hpp>
#include <utils/data.hpp>
#include <utils/data_encoding/bencode.hpp>
#include <utils/vector.hpp>
#include <utils/text/hex.hpp>
#include <utils/text/time/filename.hpp>
#include <utils/windows/ui/console.hpp>
#include <utils/FileWriter.hpp>
#include <utils/random.hpp>
#include <utils/FileReader.hpp>
#include <utils/function.hpp>

namespace program
{
	void run()
	{
		//使用ipv6
		using ip_version = utils::network::ip::version::v6;

		//准备数据: ip网络的各个类
		using Ip = utils::network::ip::Ip<ip_version>;
		using Port = utils::network::ip::Port;
		using Endpoint = utils::network::ip::Endpoint<ip_version>;

		//准备数据: udp网络的各个类
		using Udp = utils::network::udp::Udp<ip_version>;

		//准备数据: dht网络的各个类
		using NodeManager = utils::network::dht::NodeManager<ip_version>;
		using SearchManager = utils::network::dht::SearchManager<ip_version>;
		using Id = utils::network::dht::Id;
		using IdIpBinder = utils::network::dht::IdIpBinder<ip_version>;
		using PacketCodec = utils::network::dht::PacketCodec<ip_version>;
		using Node = utils::network::dht::Node<ip_version>;

		//准备数据
		NodeManager node_manager;	//节点管理器

		//设置参数
		node_manager.m_param.optional.dirname_save = u8R"(E:\zz\delete_me\260505\visit\)";

		//载入现有文件失败
		if (!node_manager.load())
		{
#if 0	//ipv4
			//手动插入最开始的节点
			node_manager.add(
			{
				utils::network::dht::Id::make::by_text_hex(u8"7d6bc4ab53e791b4c4f096819325ea4c87c22068"),
				utils::network::ip::v4::Ip::make::by_text_hex(u8"55115432"),
				(utils::u2)28012
			});
#else	//ipv6
			//手动插入最开始的节点
			node_manager.add(
			{
				Id{utils_hex_to_bytes(u8"6b5611b3bfb7b8c8372c069a1e22e540609bda5a")},
				Ip{utils_hex_to_bytes(u8"200141d002034cca0005000000000000")},	//dht.transmissionbt.com:6881
				(utils::u2)6881
			}, false);
#endif
		}

		//准备数据
		std::unique_ptr<SearchManager> search_manager;	//搜索管理器

		//自己的ip地址
		const auto my_ip = Ip{utils_hex_to_bytes(u8"24098a1a3bc1436068eb983291bf2c21")};

		//准备数据: 自己的id
		const Id my_id = [&]() -> Id
		{
			//这个版本先写死
			return Id{utils_hex_to_bytes(u8"f897dcb9c46d9e588d94f2ed5e63c25ff3104b00")};

			//准备数据
			IdIpBinder binder;

			//根据ip做出一个符合规定的id
			const auto id = binder.make_id(my_ip);

			//给出
			return id;
		}();

		//准备数据
		Id target_id;		//目标id

		//准备函数: 随机生成一个新的目标位置
		auto change_target_id = [&target_id, &search_manager, &node_manager]()
		{
			//随机生成一个目标id
			target_id.m_data.assign_by_random();

			//因为目标已更新，所以，重新创建一个搜索管理器
			search_manager.reset();
			search_manager = std::make_unique<SearchManager>(node_manager, target_id);

			//todo 更新统计

			//显示提示
			const auto console_text = u8"target_id:" + target_id.m_data.to_text_hex() + u8"\n";
			utils::windows::ui::console::write_text(console_text);
		};

		//随机生成第一个目标位置
		change_target_id();

		//准备数据：udp的传输
		Udp udp;

		//忽略网络名称错误
		udp.m_param.optional.is_ignore_error_invalid_netname = true;

		//设置回调函数: 当接收到数据时
		udp.m_param.optional.on_recv = [&node_manager](utils::data && data, const Endpoint & remote_endpoint)
		{
			//准备数据: 消息的编解码器
			PacketCodec packet_codec;

			//准备回调函数：获得到find_node的回应
			packet_codec.m_param.optional.on_message_find_node_reply = [&node_manager, &remote_endpoint](std::vector<Node> && nodes, Id && remote_id)
			{
				//把新获得的节点都保存起来

				//对于每个新获得的节点
				for(const auto & node : nodes)
				{
					//保存
					node_manager.add(node);
				}
			};
			//准备回调函数：获得到get_peers的请求
			packet_codec.m_param.optional.on_message_get_peers = [&node_manager](Id && info_hash)
			{
				//把新获得的种子信息，保存起来

				//获得一个文件名
				//const auto filename = utils::text::filename::
				//	save_as_current_time::make_by_dirname(u8R"(E:\zz\delete_me\260505\seed)", u8".txt");

				//准备数据
				const auto filename = u8R"(E:\zz\delete_me\260505\seed\seed.txt)";

				//打开文件
				utils::FileWriter output{utils::FileWriter::Mode::append, filename};

				//准备数据
				const auto text_info_hash = info_hash.m_data.to_text_hex();

				//保存
				output.text.write(u8"magnet:?xt=urn:btih:" + text_info_hash + u8"\n");
			};
			//准备回调函数：消息格式错误
			packet_codec.m_param.optional.on_message_invalid = [&node_manager, &remote_endpoint]()
			{
				//把该节点，标记为乱语节点
				node_manager.flag_talk_crazy(remote_endpoint.ip);
			};

			//解析数据
			packet_codec.decode_message(data);

			//通知节点管理器，更新此ip的最后接收时间
			node_manager.update_recv_time(remote_endpoint.ip);

			//显示提示
			{
				//准备数据
				const auto remote_ip = remote_endpoint.ip.to_text_human();
				const auto remote_port = remote_endpoint.port;

				//显示提示
				const auto console_text = utils::text::format::format(u8"recv from %s:%d\n", remote_ip.c_str(), remote_port);
				utils::windows::ui::console::write_text(console_text);
			}
		};

		while(1)
		{
			//获取下一个要询问的节点
			const auto node = search_manager->get_next_node();

			//没获取到
			if(!node)
			{
				//随机生成一个新的目标位置
				change_target_id();

				//在保存之前，让发送线程休息一下。来让发送出去的消息，有足够的时间，接收反馈。准确保存，其节点的反应。
				std::this_thread::sleep_for(std::chrono::seconds{10});

				//让节点库，保存一下
				node_manager.save();

				//重新获取
				continue;
			}

			//准备数据
			const auto & _node = node.value();

			//目标地址
			const auto & target = _node.endpoint;

			//发送的数据
			utils::data data;
			{
				//准备数据
				PacketCodec packet_codec;

				//拼出数据
				data = packet_codec.make_message_find_node(my_id, target_id);
			}

			//向其发送消息
			udp.send(target, data);

			//通知节点管理器，更新目标地址的最后发送时间
			node_manager.update_send_time(target.ip);

			//显示提示
			const auto console_text = utils::text::format::format(u8"send to %s:%d\n", target.ip.to_text_human().c_str(), target.port);
			utils::windows::ui::console::write_text(console_text);

			//休息一下
			std::this_thread::sleep_for(std::chrono::milliseconds{500});
		}
	}
}
int main()
{
	program::run();
	return 0;
}
#endif