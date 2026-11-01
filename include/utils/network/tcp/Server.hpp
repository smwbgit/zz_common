#pragma once

#include <utils/network/tcp/Connection.hpp>
#include <utils/thread/Thread.hpp>

namespace utils::network::tcp
{
	// 服务器
	template <utils::network::ip::Version ip_version>
	class Server
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
				// 有连接进入
				std::optional<std::function<void(utils::network::tcp::Connection<ip_version> && connection)>> on_new_connect;
			} m_necessary;
		} m_param;

	private:
		// 服务器线程
		std::unique_ptr<utils::thread::Thread> m_thread;
		// asio 环境上下文
        std::unique_ptr<boost::asio::io_context> m_context;
		// asio 服务器
        std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
		// 为每个新连接预先创建 socket
        std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;

	public:
		Server() = default;
		Server(const Server &) = delete;
		Server(Server &&) = delete;	// todo 让它可以移动

	public:
		Server & operator = (const Server &) = delete;
		Server & operator = (Server &&) = delete;// todo 让它可以移动

	public:
		~Server()
		{
			// 停止线程
			m_thread->wait_finished();

			// 简单检查
			utils_assert_abort(!m_context);
			utils_assert_abort(!m_acceptor);
			utils_assert_abort(!m_socket);
		}

	public:
		// 开始监听
		void start()
		{
			// 简单检查
			utils_assert(m_param.m_necessary.port);
			utils_assert(m_param.m_necessary.on_new_connect);

			// 创建 服务器线程
			m_thread = std::make_unique<utils::thread::Thread>([this](const std::atomic_bool & is_should_finish)
			{
				// 创建 环境上下文
				m_context = std::make_unique<boost::asio::io_context>();

				// 创建 服务器
				m_acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*m_context);

				// 拼出端点信息
				const auto endpoint = [&]()
				{
					// 准备数据
					const auto port = m_param.m_necessary.port.value();

					// IPv4
					if constexpr (ip_version::version == 4)
					{
						return boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port);
					}
					// IPv6
					else if constexpr (ip_version::version == 6)
					{
						return boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port);
					}
					else
					{
						// 报错
						static_assert(false);
					}
				}();

				// 生成 socket
				m_acceptor->open(endpoint.protocol());

				// 设置参数
				m_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

				// 绑定端口
				m_acceptor->bind(endpoint);

				// 开始监听
				m_acceptor->listen();

				// 开始一个异步接收
				start_one_accept();

				// 发出通知：已完成初始化
				m_thread->init_be_ready();

				// 开始循环，直到得到 结束线程 的通知
				while(1)
				{
					// 得到 结束线程 的通知
					if (is_should_finish)
						break;

					// 让 asio 获得一个时间片
					m_context->poll();

					// 短暂休息一下
					utils::this_thread::yield();
				}

				// 取消未完成的任务
				m_acceptor->close();

				// 让 asio 获得一个时间片，来完成，上一步产生的，异步回调
				m_context->poll();

				// 简单检查
				utils_assert(!m_socket);

				// 释放对象
				m_acceptor.reset();
				m_context.reset();
			});

			// 等待 服务器线程 完成初始化
			m_thread->wait_init_ready();
		}

	private:
		// 开始一个异步接收
		void start_one_accept()
		{
			// ai 生成后整理

			// 简单检查
			utils_assert(!m_socket);

			// 为每个新连接预先创建 socket
            m_socket = std::make_unique<boost::asio::ip::tcp::socket>(*m_context);

			// 开始一个异步接收
            m_acceptor->async_accept(*m_socket, [this](const boost::system::error_code & ec)
			{
				// 简单检查
				utils_assert(m_socket);
				utils_assert(!ec || ec == boost::asio::error::operation_aborted);

				// 接收成功
				if (!ec)
				{
					// 准备数据
					utils::network::tcp::Connection<ip_version> connection{std::move(m_socket)};

					// 简单检查
					utils_assert(m_param.m_necessary.on_new_connect);

					// 准备数据
					auto & on_new_connect = m_param.m_necessary.on_new_connect.value();

					// 把新连接，转交给用户
					on_new_connect(std::move(connection));
				}

				// 更新状态
				m_socket.reset();

                // 没有处于 服务器退出中
                if (ec != boost::asio::error::operation_aborted)
				{
					// 继续接收下一个连接
                    start_one_accept();
                }
            });
		}
	};
}