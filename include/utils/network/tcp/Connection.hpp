#pragma once

#include <string>
#include <vector>
#include <chrono>
//#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/asio.hpp>
//#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/assert.hpp>
#include <utils/network/Exception.hpp>
#include <utils/namespace_boost.hpp>
#include <utils/inner/reader.hpp>
#include <utils/inner/writer.hpp>
#include <utils/data.hpp>
#include <utils/network/ip.hpp>
#include <utils/network/ip/asio.hpp>

namespace utils::network::tcp
{
	namespace inner
	{
		// 一个tcp连接,可以发送或接受消息
		// note: 所有方法，均为，线程不安全，以及，同步的
		template <utils::network::ip::Version ip_version>
		class Connection
		{
		private:
			std::unique_ptr<boost::asio::io_context> m_context;	//如果直接传入socket，则此项为空。直接使用socket的context
			std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;	//如果直接传入socket, 把socket保存在这里。否则，创建一个socket。
			utils::data m_data;
			uint64_t m_read_pos{};
			uint64_t m_write_pos{};

		public:
			// 做为客户端
			// ip: 形如 "192.168.0.1"（ipv4） 或者 "f021::c000"（ipv6）
			Connection(const std::u8string & ip, u2 port)
			{
				// 创建环境
				m_context = std::make_unique<boost::asio::io_context>();

				// 创建 socket
				m_socket = std::make_unique<boost::asio::ip::tcp::socket>(*m_context);

				// 准备数据
				utils::network::ip::Ip<ip_version> ip_2{ ip };

				// 准备数据
				utils::network::ip::Endpoint<ip_version> server{ ip_2, port };

				// 简单转换
				utils::network::ip::asio::TcpEndpoint<ip_version> server_2{ server };

				try
				{
					// 连接
					m_socket->connect(server_2);
				}
				catch (boost::system::system_error & error)
				{
					// 无权限
					if (error.code() == boost::asio::error::no_permission)
					{
						// 报错
						throw utils::network::Exception{};
					}

					// 报错
					throw utils::network::Exception{};
				}
			}
			// 做为服务器，监听时的，一个连入连接
			Connection(std::unique_ptr<boost::asio::ip::tcp::socket> socket)
			{
				// 保存
				m_socket = std::move(socket);
			}
			~Connection()
			{
				// socket 还未关闭
				if (m_socket && m_socket->is_open())
				{
					// 准备数据
					boost::system::error_code ec{};

					// 关闭
					m_socket->shutdown(m_socket->shutdown_both, ec);

					// 简单检查
					utils_assert_abort(
						!ec ||
						ec == boost::asio::error::not_connected ||	// 之前已经断开
						ec == boost::asio::error::bad_descriptor	// 之前从未连接
					);
				}
			}

		public:
			// 获取内部表示
			boost::asio::ip::tcp::socket & get_native_socket()
			{
				// 简单检查
				utils_assert(m_socket);

				return *m_socket;
			}

#pragma region 提供给utils::inner::reader
			void read(_Out_ void* data, _In_ u8 size)
			{
				//确保有足够数据
				read_until_this_size(m_read_pos + size);

				//给出
				m_data.subdata(data, m_read_pos, size);

				//更新状态
				m_read_pos += size;
			}
			void tellg(_Out_ u8 & pos)
			{
				pos = m_read_pos;
			}
			void seekg(_In_ u8 pos)
			{
				//确保有足够数据
				read_until_this_size(pos);

				//移动位置
				m_read_pos = pos;
			}
			void get_pool_size(_Out_ u8 & pool_size)
			{
				//确保已读取所有数据
				read_until_peer_close();

				//给出
				pool_size = m_data.size();
			}
#pragma endregion
#pragma region 提供给utils::inner::writer
			//note: 发送完毕后才返回
			void write(_In_ const void* data, _In_ u8 size)
			{
				//写入
				try
				{
					const auto writed_size = ::boost::asio::write(*m_socket, ::boost::asio::buffer(data, size));

					//简单检查
					utils_assert(size == writed_size);
				}
				catch(boost::system::system_error &)
				{
					//报错
					throw utils::network::Exception{};
				}

				//更新状态
				m_write_pos += size;
			}
			void tellp(_Out_ u8 & pos)
			{
				//给出
				pos = m_write_pos;
			}
#pragma endregion

#pragma region 读取
			// 读取直到数据足够
			// note: 不移动读取位置
			void read_until_this_size(_In_ const u8 size)
			{
				//已足够
				if (m_data.size() >= size)
					return;

				//还需读的数据大小
				const auto _size = size - m_data.size();

				//准备数据
				utils::data data;
				data.resize(_size);

				//读取
				try
				{
					//读取
					const auto readed_size = ::boost::asio::read(*m_socket, ::boost::asio::buffer(data.addr(), data.size()));

					//简单检查
					utils_assert(_size == readed_size);
				}
				catch(boost::system::system_error & e)
				{
					//报错
					if (e.code() == boost::asio::error::eof)
						throw utils::network::Exception{};	//todo utils::network::error_code::eof
					else
						throw utils::network::Exception{};
				}

				//保存
				m_data.append(data);
			}
			// 确保已读取所有数据
			// note: 不移动读取位置
			void read_until_peer_close()
			{
				// 已读取所有
				if (!m_socket->is_open())
					return;

				// 准备数据
				utils::data buffer;
				buffer.resize(1024);

				// 循环读取，直到全部读取
				while(1)
				{
					// 读取
					u8 readed_size{};
					bool is_peer_close{};
					try
					{
						// 读取
						readed_size = m_socket->read_some(::boost::asio::buffer(buffer.addr(), buffer.size()));
					}
					catch (boost::system::system_error & e)
					{
						// for debug
						const auto _code = e.code();
						const auto _msg = _code.message();

						if (e.code() == boost::asio::error::eof)
							is_peer_close = true;
						else if (e.code() == boost::asio::error::connection_reset)
						{
							//远程主机强迫关闭了一个现有的连接

							is_peer_close = true;
						}
						else
						{
							//发生了其他错误
							throw utils::network::Exception{};
						}
					}

					// 对方关闭
					if (readed_size == 0)	//todo: 确认下是否可用此条件来检测正常的服务器断开
					{
						//简单检查
						utils_assert(is_peer_close);

						break;
					}

					//保存
					m_data.append(buffer, readed_size);
				}
			}
#pragma endregion

#if 0
			//获取m_context
			boost::asio::io_context& get_m_context()
			{
				return (boost::asio::io_context &) m_socket->get_executor();
			}
#endif
		};
	}

	template <utils::network::ip::Version ip_version>
	using Connection = utils::inner::writer::WithCommonFunctions<utils::inner::reader::WithCommonFunctions<inner::Connection<ip_version>>>;
}