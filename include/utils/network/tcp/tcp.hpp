#pragma once

#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "utils/assert.hpp"

#pragma comment(lib, "ws2_32.lib")

namespace utils::network::tcp
{
	//发送一次数据，然后接收，直到服务器端主动断开连接
	//ip: 服务器ip
	//port: 服务器port
	//data_send: 要发送的数据
	//data_recv: 接收到的数据
	void send_only_once_and_recv(
		_In_ const std::string & ip,
		_In_ const uint16_t port,
		_In_ const std::vector<uint8_t> & data_send,
		_Out_ std::vector<uint8_t> & data_recv)
	{
		WSADATA data{};
		utils_assert(0 == WSAStartup(MAKEWORD(2, 2), &data));	//todo move this and WSACleanup() to somewhere, so that these can be done only once

		SOCKET s{};
		s = socket(AF_INET, SOCK_STREAM, 0);
		utils_assert(s != INVALID_SOCKET);

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
		addr.sin_port = htons(port);

		utils_assert(0 == connect(s, (SOCKADDR*)&addr, sizeof(addr)));

		uint64_t offset = 0;
		while (1)
		{
			auto re = send(s, (const char*)data_send.data() + offset, (int)(data_send.size() - offset), 0);
			utils_assert(re != SOCKET_ERROR);

			//简单检查
			utils_assert(re > 0);
			utils_assert(re <= data_send.size() - offset);

			//更新状态
			offset += re;

			//全部发完
			if (offset >= data_send.size())
				break;
		}

		data_recv.clear();
		std::vector<uint8_t> buffer;
		buffer.resize(1024);
		while (1)
		{
			auto re = recv(s, (char*)buffer.data(), (int)buffer.size(), 0);
			utils_assert(re >= 0);

			if (re > 0)
			{
				//简单检查
				utils_assert(re <= buffer.size());

				//保存
				data_recv.insert(data_recv.end(), buffer.begin(), buffer.begin() + re);
				continue;
			}

			//连接断开
			break;
		}

		utils_assert(0 == closesocket(s));
		utils_assert(0 == WSACleanup());
	}
}