#pragma once

#include <vector>
#include "utils/string.hpp"
#include "utils/network/tcp/Connection.hpp"
#include "utils/DataReader.hpp"
#include "utils/DataWriter.hpp"
#include "utils/bit.hpp"

namespace utils::protocol::modbus_tcp
{
	//ModbusTcp服务器
	class s_modbus_tcp_server
	{
	public:
		using addr_t = uint16_t;
		using n_t = uint64_t;

	private:
		std::vector<uint8_t> m_block[4];
		boost::asio::io_context m_io_context;

	public:
		s_modbus_tcp_server()
		{
			m_block[0].resize(1250);	//线圈
			m_block[1].resize(1250);	//离散量
			m_block[2].resize(20000);	//保持寄存器
			m_block[3].resize(20000);	//输入寄存器
		}
		~s_modbus_tcp_server()
		{

		}

#pragma region 本机
		//设置位
		void set_bit(addr_t addr, bool bit);
		//设置寄存器
		void set_register(addr_t addr, uint16_t value);
		//设置位
		void set_bits(addr_t addr, const std::vector<bool>& bits);
		//设置寄存器
		void set_registers(addr_t addr, const std::vector<uint16_t>& values);

		//获取位
		bool get_bit(addr_t addr) const;
		//获取寄存器
		uint16_t get_register(addr_t addr) const;
		//获取位
		std::vector<bool> get_bits(addr_t addr, n_t n) const;
		//获取寄存器
		std::vector<uint16_t> get_registers(addr_t addr, n_t n) const;
#pragma endregion

#pragma region 网络
		//开始监听
		bool start(uint16_t port)
		{
			using tcp = boost::asio::ip::tcp;

			//绑定
			tcp::acceptor acceptor{m_io_context};
			try
			{
				tcp::endpoint endpoint(tcp::v4(), port);
				acceptor.open(endpoint.protocol());
				acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
				acceptor.bind(endpoint);
				acceptor.listen();
			}
			catch (boost::system::system_error &)
			{
				return false;
			}

			//监听
			std::thread{ [=](tcp::acceptor acceptor)
			{
				while (1)
				{
					std::thread{ [=](tcp::socket socket)
					{
#ifdef zz_ndebug
						try
						{
#endif
							session(std::move(socket));
#ifdef zz_ndebug
						}
						catch (...)
						{
							__debugbreak();

						}
#endif
					}, acceptor.accept() }.detach();
				}
			}, std::move(acceptor)}.detach();
			return true;
		}
#pragma endregion

	private:
		void session(boost::asio::ip::tcp::socket socket)
		{
			utils::network::tcp::Connection connection{ std::move(socket) };

			while (1)
			{
				//接收
				uint16_t 事务处理标识{};
				uint16_t 协议标识{};
				uint16_t 长度{};
				uint8_t 单元标识{};
				uint8_t 功能码{};
				utils::data 数据;
				{
					connection.big_endian_r.read(事务处理标识);
					connection.big_endian_r.read(协议标识);
					connection.big_endian_r.read(长度);

					//简单检查
					utils_assert(协议标识 == 0);
					utils_assert(长度 >= 2);

					connection.read(单元标识);
					connection.read(功能码);

					connection.read(数据, 长度 - 2);
				}

				//处理
				switch (功能码)
				{
				case 0x01://读线圈
				case 0x02://读离散量
				case 0x03://读保持寄存器
				case 0x04://读输入寄存器
				{
					//提取
					uint16_t 起始地址{};
					uint16_t 读取数量{};
					{
						utils::DataReader data{数据};
						data.big_endian.read(起始地址);
						data.big_endian.read(读取数量);

						//简单检查
						utils_assert(起始地址 < 10000);
						utils_assert(读取数量 < 10000);
						utils_assert(起始地址 + 读取数量 <= 10000);
					}


					//读取数据
					utils::data wanted_data;
					{
						if (功能码 == 0x01 || 功能码 == 0x02)
						{
							wanted_data.resize((读取数量 + 7) / 8);

							for (uint64_t i = 0; i < 读取数量; i++)
							{
								//读取一位
								bool bit = utils::bit::get_bit(m_block[功能码 - 1], 起始地址 + i);
								//保存
								utils::bit::set_bit(wanted_data, i, bit);
							}
						}
						else
						{
							//读取
							std::vector<uint16_t> values;
							utils::DataReader reader{m_block[功能码 - 1]};
							reader.seekg(起始地址 * 2);
							reader.read(values, 读取数量);

							//保存
							utils::DataWriter writer;
							writer.big_endian.write(values);
							writer.get_data_and_clear(wanted_data);
						}
					}

					//发送
					{
						//简单检查
						utils_assert(wanted_data.size() < 256);

						//准备数据
						uint8_t 字节计数 = (uint8_t)wanted_data.size();

						//发送
						connection.big_endian.write(事务处理标识);
						connection.big_endian.write(协议标识);
						connection.big_endian.write((uint16_t)(3 + 字节计数));	//长度
						connection.big_endian.write(单元标识);
						connection.big_endian.write(功能码);
						connection.big_endian.write(字节计数);
						connection.write(wanted_data);
					}
					break;
				}
				case 0x05://写入单个线圈
				case 0x06://写入单个保持寄存器
				{
					//提取
					uint16_t 地址{};
					uint16_t 值{};
					{
						utils::DataReader data{数据};
						data.big_endian.read(地址);
						data.big_endian.read(值);

						//简单检查
						utils_assert(地址 < 10000);
					}

					//写入数据
					if (功能码 == 0x05)
					{
						//简单检查
						utils_assert(值 == 0xFF00 || 值 == 0x0000);

						utils::bit::set_bit(m_block[0], 地址, 值 == 0xFF00);
					}
					else
					{
						*(uint16_t*)&m_block[2][地址 * 2] = 值;
					}

					//发送
					connection.big_endian.write(事务处理标识);
					connection.big_endian.write(协议标识);
					connection.big_endian.write(长度);
					connection.big_endian.write(单元标识);
					connection.big_endian.write(功能码);
					connection.big_endian.write(地址);
					connection.big_endian.write(值);
					break;
				}
				case 0x0f://写多个线圈
				case 0x10://写多个保持寄存器
				{
					//提取
					uint16_t 起始地址{};
					uint16_t 写入数量{};
					uint8_t 字节数{};
					utils::data 写入的值;
					{
						utils::DataReader data{数据};
						data.big_endian.read(起始地址);
						data.big_endian.read(写入数量);
						data.big_endian.read(字节数);
						data.read_rest(写入的值);

						//简单检查
						utils_assert(起始地址 < 10000);
						utils_assert(写入数量 < 10000);
						utils_assert(起始地址 + 写入数量 <= 10000);
						utils_assert(写入的值.size() == 字节数);

						//检查
						if (功能码 == 0x0f)
							utils_assert(字节数 == (写入数量+7)/8);
						else
							utils_assert(字节数 == 写入数量*2);
					}

					//写入数据
					if (功能码 == 0x0f)
					{
						for (uint64_t i = 0; i < 写入数量; i++)
						{
							//从传过来的数据中获取一位
							bool bit = utils::bit::get_bit(写入的值, i);
							//保存
							utils::bit::set_bit(m_block[0], 起始地址 + i, bit);
						}
					}
					else
					{
						utils::DataReader data{写入的值};
						for (uint64_t i = 0; i < 写入数量; i++)
						{
							//从传过来的数据中获取一个short
							uint16_t value{};
							data.big_endian.read(value);
							//保存
							*(uint16_t*)&m_block[2][(起始地址 + i) * 2] = value;
						}
					}

					//发送
					connection.big_endian.write(事务处理标识);
					connection.big_endian.write(协议标识);
					connection.big_endian.write((uint16_t)6);	//长度
					connection.big_endian.write(单元标识);
					connection.big_endian.write(功能码);
					connection.big_endian.write(起始地址);
					connection.big_endian.write(写入数量);
					break;
				}
				}
			}
		}
	};

}