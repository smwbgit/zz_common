#pragma once

#define OPENSSL_SUPPRESS_DEPRECATED

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")

#include <string>
#include <vector>
#include <chrono>
#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/assert.hpp>
#include <utils/network/Exception.hpp>
#include <utils/namespace_boost.hpp>
#include <utils/inner/reader.hpp>
#include <utils/inner/writer.hpp>
#include <utils/data.hpp>

namespace utils::network::tls
{
	namespace inner
	{
		//建立一个tls连接,可以发送或接受消息
		class ConnectionBase
		{
		private:
			boost::asio::io_context m_context;
			boost::asio::ssl::context m_ssl_context{boost::asio::ssl::context::sslv23};
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket{m_context, m_ssl_context};
			utils::data m_data;
			uint64_t m_read_pos{};
			uint64_t m_write_pos{};

		public:
			//ip: 形如 192.168.0.1
			ConnectionBase(const std::u8string & ip, uint16_t port, const std::u8string & cert_filename)
			{
				boost::system::error_code ec;

				//准备数据
				const auto ip_2 = utils::u8string(ip).cast_to_string();
				const auto cert_filename_2 = utils::u8string(cert_filename).cast_to_string();

				//载入证书文件
				m_ssl_context.load_verify_file(cert_filename_2, ec);
				if (ec)
					throw utils::network::Exception{};
				
				//准备数据
				auto _ip = boost::asio::ip::address_v4::from_string(ip_2, ec);
				if (ec)
					throw utils::network::Exception{};
				boost::asio::ip::tcp::endpoint server{_ip, port};

				//连接
				m_socket.lowest_layer().connect(server, ec);
				if (ec)
					throw utils::network::Exception{};

				//握手
				m_socket.handshake(boost::asio::ssl::stream_base::client, ec);
				if (ec)
					throw utils::network::Exception{};

//				try
//				{
//
//					//连接
//					auto _ip = boost::asio::ip::address_v4::from_string(ip);
//					boost::asio::ip::tcp::endpoint server{_ip, port};
//					m_socket.lowest_layer().connect(server);
//					m_socket.handshake(boost::asio::ssl::stream_base::client);
//				}
//				catch(boost::system::system_error & e [[maybe_unused]])
//				{
//					throw utils::network::Exception{};
//				}
			}
#pragma region 读取
			//读取直到数据足够
			void read_until_this_size(_In_ uint64_t size)
			{
				//已足够
				if (m_data.size() >= size)
					return;

				//还需读的数据大小
				uint64_t _size = size - m_data.size();

				//准备数据
				std::vector<uint8_t> data;
				data.resize(_size);

				//读取
				try
				{
					auto readed_size = boost::asio::read(m_socket, boost::asio::buffer(data.data(), data.size()));

					//简单检查
					utils_assert(_size == readed_size);
				}
				catch(boost::system::system_error &)
				{
					throw utils::network::Exception{};
				}

				//保存
				m_data.append(data);
			}
			//确保已读取所有数据
			void read_until_peer_close()
			{
				//已读取所有
				if (m_socket.lowest_layer().is_open() == false)
					return;

				//准备数据
				std::vector<uint8_t> buffer;
				buffer.resize(1024);

				while(1)
				{
					//读取
					uint64_t readed_size{};
					bool is_peer_close{};
					try
					{
						readed_size = m_socket.read_some(boost::asio::buffer(buffer.data(), buffer.size()));
					}
					catch(boost::system::system_error & e  [[maybe_unused]])
					{
						if (e.code() == boost::asio::error::eof)
							is_peer_close = true;
						else
						{
							//发生了其他错误
							throw utils::network::Exception{};
						}
					}

					//对方关闭
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
#pragma region 写入
			//写入数据
			//note: 发送完毕后才返回
			void _write_data(_In_ const void* data, _In_ uint64_t size)
			{
				//写入
				try
				{
					auto writed_size = boost::asio::write(m_socket, boost::asio::buffer(data, size));

					//简单检查
					utils_assert(size == writed_size);
				}
				catch(boost::system::system_error &)
				{
					throw utils::network::Exception{};
				}

				//更新状态
				m_write_pos += size;
			}
#pragma endregion
#pragma region 提供给utils::inner::reader
			void read(_Out_ void* data, _In_ uint64_t size)
			{
				//读取足够的数据
				uint64_t start_pos{};
				tellg(start_pos);
				seekg(start_pos + size);

				//给出
				m_data.subdata(data, start_pos, size);
			}
			void tellg(_Out_ uint64_t & pos)
			{
				pos = m_read_pos;
			}
			void seekg(_In_ uint64_t pos)
			{
				//确保有足够数据
				read_until_this_size(pos);

				//移动位置
				m_read_pos = pos;
			}
			void get_pool_size(_Out_ uint64_t & pool_size)
			{
				//确保已读取所有数据
				read_until_peer_close();

				//给出
				pool_size = m_data.size();
			}
#pragma endregion
#pragma region 提供给utils::inner::writer
			void write(_In_ const void* data, _In_ uint64_t size)
			{
				//转发
				_write_data(data, size);
			}
			void tellp(_Out_ uint64_t & pos)
			{
				//给出
				pos = m_write_pos;
			}
#pragma endregion
		};
	}

	using Connection = utils::inner::writer::WithCommonFunctions<utils::inner::reader::WithCommonFunctions<inner::ConnectionBase>>;
}