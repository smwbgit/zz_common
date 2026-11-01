#pragma once

#include <boost/asio.hpp>
#include <utils/data.hpp>
#include <utils/network/ip.hpp>
#include <utils/network/ip/asio.hpp>
#include <utils/function.hpp>

namespace utils::network::udp
{
	// 负责 udp 的收发
	// note: 所有函数，均为，线程非安全，以及，同步的
	template<utils::network::ip::Version ip_version>
	class Connection
	{
	public:
		// 参数
		struct s_param
		{
			// 必填参数
			struct s_necessary
			{
				// 端口
				std::optional<u2> port;
			} necessary;

			// 可选参数
			struct s_optional
			{
				// 绑定时，向哪个 地址/网卡 绑定
				std::optional<utils::network::ip::Ip<ip_version>> ip_bind;
				// 是否忽略发送数据时的 目标地址的错误而导致的 发不出去
				bool is_ignore_error_invalid_argument{};
			} optional;
		};
		s_param m_param;

	private:
		// asio 环境上下文
		boost::asio::io_context m_context;

		// 负责收发的 socket
		boost::asio::ip::udp::socket m_socket{m_context};

		//构造函数
	public:
		Connection() = default;
		Connection(const Connection &) = delete;
		Connection(Connection &&) = delete;	//todo 改为可移动的
		Connection & operator = (const Connection &) = delete;
		Connection & operator = (Connection &&) = delete;	//todo 改为可移动的

	public:
		// 绑定端口
		void bind()
		{
			// 简单检查
			utils_assert(m_param.necessary.port);

			// 准备数据
			const auto port = m_param.necessary.port.value();

			// 简单获取
			const auto protocol = [&]()
			{
				if constexpr (ip_version::version == 4)
				{
					return boost::asio::ip::udp::v4();
				}
				else if constexpr (ip_version::version == 6)
				{
					return boost::asio::ip::udp::v6();
				}
				else
				{
					//报错
					static_assert(false);
				}
			}();

			// 准备数据
			boost::system::error_code ec{};

			// 创建 socket
			m_socket.open(protocol, ec);

			// 简单检查
			utils_assert(!ec);

			// 准备数据
			const auto endpoint = [&]() -> boost::asio::ip::udp::endpoint
			{
				// 用户指定了，绑定时要用的ip
				if (m_param.optional.ip_bind)
				{
					// 准备数据
					const auto & ip_2 = m_param.optional.ip_bind.value();

					// 简单转换: 转为 asio 库的表示
					utils::network::ip::asio::Ip<ip_version> ip_3{ ip_2 };

					// 给出
					return {ip_3, port};
				}
				// 否则
				else
				{
					// 绑定到任意地址
					return {protocol, port};
				}
			}();

			// 绑定端口
			m_socket.bind(endpoint, ec);

			// 简单检查
			utils_assert(!ec);
		}
		// 获取绑定信息
		utils::network::ip::Endpoint<ip_version> get_local_endpoint()
		{
			// 准备数据
			boost::system::error_code ec{};

			// 获取绑定信息
			const auto endpoint = m_socket.local_endpoint(ec);

			// 简单检查
			utils_assert(!ec);

			// 简单转换
			const utils::network::ip::asio::UdpEndpoint<ip_version> endpoint_2{ endpoint };

			// 给出
			return endpoint_2.get();
		}
		// 发送一个数据包
		void send(const utils::network::ip::Endpoint<ip_version> & endpoint, utils::data_view data)
		{
			// 简单检查
			utils_assert(m_socket.is_open());

			// 简单转换：转为asio库的表示方式
			utils::network::ip::asio::UdpEndpoint<ip_version> endpoint_2{ endpoint };

			// 准备数据
			boost::system::error_code ec{};

			// 发送
			m_socket.send_to(boost::asio::buffer(data.addr(), data.size()), endpoint_2, {}, ec);

			// 检查发送结果
			[&]()
			{
				// 网络地址格式不正确
				if (ec == boost::asio::error::invalid_argument)
				{
					// 用户要求，忽略此错误
					if(m_param.optional.is_ignore_error_invalid_argument)
					{
						// 清空
						ec.clear();

						// 通过检查
						return;
					}
				}

				// 简单检查
				utils_assert(!ec);
			}();
		}
		// 接收一个数据包
		void receive(std::function<void(utils::data && data, utils::network::ip::Endpoint<ip_version> && from)> meet_one_packet)
		{
			// 简单检查
			utils_assert(m_socket.is_open());

			// 准备数据: 接收缓冲区
			utils::data buffer;
			buffer.resize(65536);	//note: 直接开到 UDP 单个报文大小的上限，来能始终完整接收一个报文

			// 准备数据: 对方端点
			boost::asio::ip::udp::endpoint from{};

			// 准备数据
			boost::system::error_code ec{};

			// 接收
			const u8 received_size = m_socket.receive_from(
				boost::asio::buffer(buffer.addr(), buffer.size()),
				from, {}, ec);
			
			// 简单检查
			utils_assert(!ec);
			utils_assert(received_size > 0);
			utils_assert(received_size <= buffer.size());

			// 简单转换
			const utils::network::ip::asio::UdpEndpoint<ip_version> from_2{ from };

			// 简单转换：转为 zz_common 库的表示方式
			auto from_3 = from_2.get();

			// 改为实际获取到的大小
			buffer.resize(received_size);

			// 给出
			meet_one_packet(std::move(buffer), std::move(from_3));
		}
		// 是否存在待接收的数据包
		// note: 若返回true，则调用 receive 不会阻塞
		bool is_exist_new_packet()
		{
			// 简单检查
			utils_assert(m_socket.is_open());

			// 准备数据
			boost::system::error_code ec{};

			// 查询
			const auto size = m_socket.available(ec);
			
			// 简单检查
			utils_assert(!ec);
			utils_assert(size >= 0 && size <= 65536);

			// 给出
			return size;
		}
	};

#if 0	//似乎，是一个，异步的，实现
	//表示一个udp端口
	//note: 可以通过此端口，进行，异步的收发
	//todo: 把网络部分（asio库）可能抛出异常的地方，catch一下
	template<utils::network::ip::version::Version ip_version>
	class Connection
	{
	public:
		using Endpoint = utils::network::ip::Endpoint<ip_version>;
	private:
		using boost_udp = boost::asio::ip::udp;

	public:
		struct s_param
		{
			struct s_optional
			{
				//接收到数据时，的回调函数
				std::function<void(utils::data && data, const Endpoint & remote_endpoint)> on_recv;
				//是否忽略发送数据时的，地址错误而导致的，发不出去
				bool is_ignore_error_invalid_netname{};
			} optional;
		};
		s_param m_param;

	private:
		utils::thread::Thread m_thread;

		struct s_private_data
		{
			boost::asio::io_context m_io;
			boost_udp::socket m_socket{ m_io, []()
				{
					if constexpr(ip_version::version == 4)
					{
						return boost_udp::endpoint{boost_udp::v4(), 0};
					}
					else if constexpr(ip_version::version == 6)
					{
						return boost_udp::endpoint{boost_udp::v6(), 0};
					}
					else
					{
						static_assert(false);
					}
				}()};
			boost_udp::endpoint m_remote_endpoint;
			utils::data m_buffer;
		};
		std::unique_ptr<s_private_data> m_p;

	public:
		Udp()
			: m_thread{[this](){on_thread();}}
		{
			m_thread.wait_init_ready();
		}
		void send(const Endpoint & target, utils::data_view data)
		{
			namespace asio = boost::asio;

			//简单转换：转为asio库的表示方式
			boost_udp::endpoint _target = utils::product::boost::asio::ip::
				convert_with_utils::from_utils(target);

			//因为数据可能不会被立刻发送。所以此处，保留一份数据的副本。
			utils::data* _data = new utils::data{data.get_data()};

			//从用户线程，转为网络收发线程
			asio::post(m_p->m_io, [this, _target, _data]()
			{
				//开始异步发送
				m_p->m_socket.async_send_to(
					asio::buffer(_data->m_data), _target,
					[this, _data, _target](boost::system::error_code ec, std::size_t n)
					{
						//把发送缓冲区释放掉
						delete _data;

						//被正常取消，直接返回
						if (ec == boost::asio::error::operation_aborted)
							return;

						//发送失败
						if (ec)
						{
							//网络地址格式不正确
							if (ec == boost::asio::error::invalid_argument)
							{
								//用户要求，忽略此错误
								if(m_param.optional.is_ignore_error_invalid_netname)
								{
									//直接返回
									return;
								}
							}

							const auto a = ec.value();
							const auto b = ec.category().name();
							const auto c = ec.message();
							const auto d = _target;
							const auto e = _target.address().to_string();
							const auto f = _target.port();

							//todo 看看这里怎么写
							utils_throw();
						}
					});
			});
		}
	private:
		void on_thread()
		{
			//准备环境
			//note: 写在这里是因为，想让socket和context它们，在自己的线程上创建和清理。
			m_p = std::make_unique<s_private_data>();

			//开始异步接收
			start_receive();

			//向外报告，线程准备完毕
			m_thread.init_be_ready();

			while(1)
			{
				//应该结束线程
				if(m_thread.is_should_finish())
					break;

				//处理新来的事件
				m_p->m_io.poll();

				//休息一下
				utils::this_thread::yield();
			}

			//清理环境
			m_p.reset();
		}
		//开始异步接收
		void start_receive()
		{
			namespace asio = boost::asio;

			//准备数据
			m_p->m_buffer.clear();	//note: 清0一下，来让刚才的数据消失
			m_p->m_buffer.resize(65536);	//note: 直接开到UDP单个报文大小的上限，来能始终完整接收一个报文

			//开始异步接收
			m_p->m_socket.async_receive_from(
				asio::buffer(m_p->m_buffer.m_data), m_p->m_remote_endpoint,
				[this](boost::system::error_code ec, std::size_t bytes)
				{
					//接到数据
					if (!ec && bytes > 0)
					{
						//调整到实际接收数量上
						m_p->m_buffer.resize(bytes);

						//简单转换：转为utils库的表示方式
						const auto remote_endpoint = utils::product::boost::asio::ip::
							convert_with_utils::to_utils<ip_version>(m_p->m_remote_endpoint);

						//把数据传给用户
						utils::function(m_param.optional.on_recv).
							call_if_exist(std::move(m_p->m_buffer), remote_endpoint);

						//清空数据
						m_p->m_buffer.clear();
					}

					//应该结束线程，则不再接收
					if(m_thread.is_should_finish())
						return;

					//被正常取消，也不再接收
					if (ec == boost::asio::error::operation_aborted)
						return;

					//连接没问题
					if (m_p->m_socket.is_open())
					{
						//再次启动异步接收
						start_receive();
					}
				});
		}
	};
#endif
}