#pragma once

#include <vector>
#include "utils/string.hpp"
#include "utils/map.hpp"
#include "utils/network/tcp/Connection.hpp"

namespace utils::protocol::http
{
	//客户端
	class Client
	{
	private:
		utils::network::tcp::Connection m_connection;
		utils::map<utils::string, utils::string> m_header_fields;

	private:
		//设置报文头
		static void set(_In_ utils::string_view name, _In_ std::string_view value, _Inout_ utils::map<utils::string, utils::string> & header_fields)
		{
			//字段名转为小写
			utils::string _name = name.to_lower();

			//保存
			header_fields[_name] = value;
		}
	public:
		//连接服务器
		void connect(_In_ std::string_view ip, _In_ uint16_t port)
		{
			return m_connection.connect(ip, port);
		}
		//设置报文头
		//example: set("Host", "www.123.org");
		void set(_In_ std::string_view name, _In_ std::string_view value)
		{
			set(name, value, m_header_fields);
		}
		//通过post，向服务器发送数据，并接收服务器返回的数据
		//note: "content-length"字段会自动调整为合适值
		void post(_In_ std::string_view page, _In_ utils::data_view body, _Out_ uint64_t & status_code, _Out_ std::string & return_data)
		{
			//发送
			{
				auto send_one_line = [&](const std::string & line)
				{
					m_connection.write_text(line + "\r\n");
				};

				//简单检查
				utils_assert(body.empty() == false);

				//method
				send_one_line(std::string("POST ") + (std::string)page + " HTTP/1.1");

				//fields
				{
					//消息体大小
					{
						if (body.size() > 0)	//todo 看下这里是和POST相关还是和m_body的大小相关
							set("content-length", std::to_string(body.size()));
						else
							m_header_fields.erase("content-length");
					}

					for (const auto & e : m_header_fields)
						send_one_line(e.first + ": " + e.second);
				}

				//head finish
				send_one_line("");

				//body
				m_connection.write(body);
			}

			//接收：状态码
			utils::string first_line;
			m_connection.read_one_field(first_line, "\r\n");
			{
				auto words = first_line.string_view().split(' ');
				
				//简单检查
				utils_assert(words.size() >= 3);
				utils_assert(words[0] == "HTTP/1.1");
				utils_assert(words[1].empty() == false);
				utils_assert(words[2].empty() == false);

				words[1].cast_to_this_type(status_code);
			}
			//接收：fields
			utils::map<utils::string, utils::string> _header_fields;
			while (1)
			{
				utils::string _line;
				m_connection.read_one_field(_line, "\r\n");

				if (_line.empty())
					break;
				
				//提取
				std::string_view name;
				std::string_view value;
				_line.string_view().solve(name, ": ", value);

				//保存
				set(name, value, _header_fields);

				//简单检查
				utils_assert(name.empty() == false);
				utils_assert(value.empty() == false);
			}

			//接收：body
			if (_header_fields.exist("transfer-encoding") && _header_fields["transfer-encoding"] == "chunked")
			{
				//简单检查
				utils_assert(false == _header_fields.exist("content-length"));

				while(1)
				{
					utils::string _line;
					m_connection.read_one_field(_line, "\r\n");
				
					uint32_t size{};
					_line.string_view().cast_to_this_type(size, 16);

					if (size == 0)
						break;

					std::string _data;
					m_connection.read_text(_data, size);
					m_connection.read_this_text("\r\n");

					return_data.insert(return_data.end(), _data.begin(), _data.end());
				}
			}
			else if (_header_fields.exist("content-length"))
			{
				uint64_t length{};
				_header_fields["content-length"].string_view().cast_to_this_type(length);

				m_connection.read_text(return_data, length);
			}
			else
			{
				utils_throw();
			}
		}
	};

	//服务器
	class Server
	{
	private:
		utils::network::tcp::Connection m_connection;
		utils::map<utils::string, utils::string> m_header_fields;

	public:
	};
}
