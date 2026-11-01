#pragma once

#include <boost/asio.hpp>
#include <utils/network/ip.hpp>

//简单转换：在asio库和utils库之间，简单转换，表示方式
namespace utils::product::boost::asio::ip::convert_with_utils
{
	//简单转换：Ip
	template<utils::network::ip::version::Version ip_version>
	inline ::boost::asio::ip::address from_utils(const utils::network::ip::Ip<ip_version> & ip)
	{
		if constexpr(ip_version::version == 4)
		{
			//转换
			const auto ip_1 = ::boost::asio::ip::address_v4{ip.m_data};
			const auto ip_2 = ::boost::asio::ip::address{ip_1};

			//给出
			return ip_2;
		}
		else if constexpr(ip_version::version == 6)
		{
			//转换
			const auto ip_1 = ::boost::asio::ip::address_v6{ip.m_data.get_native()};
			const auto ip_2 = ::boost::asio::ip::address{ip_1};

			//给出
			return ip_2;
		}
		else
		{
			//报错
			static_assert(false);
		}
	}
	//简单转换：Ip
	template<utils::network::ip::version::Version ip_version>
	inline utils::network::ip::Ip<ip_version> to_utils(const ::boost::asio::ip::address & ip)
	{
		if constexpr(ip_version::version == 4)
		{
			//简单检查
			utils_assert(ip.is_v4());

			//转换
			const auto ip_1 = ip.to_v4();
			const auto ip_2 = ip_1.to_bytes();
			const auto ip_3 = utils::network::ip::Ip<ip_version>{ip_2};

			//给出
			return ip_3;
		}
		else if constexpr(ip_version::version == 6)
		{
			//简单检查
			utils_assert(ip.is_v6());

			//转换
			const auto ip_1 = ip.to_v6();
			const auto ip_2 = ip_1.to_bytes();
			const auto ip_3 = utils::network::ip::Ip<ip_version>{ip_2};

			//给出
			return ip_3;
		}
		else
		{
			//报错
			static_assert(false);
		}
	}
	//简单转换：Endpoint
	template<utils::network::ip::version::Version ip_version>
	inline ::boost::asio::ip::udp::endpoint from_utils(const utils::network::ip::Endpoint<ip_version> & endpoint)
	{
		//转换每个成员
		const auto _ip = from_utils(endpoint.ip);
		const auto _port = endpoint.port.m_data;

		//拼出
		const ::boost::asio::ip::udp::endpoint _endpoint{_ip, _port};

		//给出
		return _endpoint;
	}
	//简单转换：Endpoint
	template<utils::network::ip::version::Version ip_version>
	inline utils::network::ip::Endpoint<ip_version> to_utils(const ::boost::asio::ip::udp::endpoint & endpoint)
	{
		//转换每个成员
		const auto _ip = to_utils<ip_version>(endpoint.address());
		const auto _port = endpoint.port();

		//拼出
		const utils::network::ip::Endpoint<ip_version> _endpoint{_ip, _port};

		//给出
		return _endpoint;
	}
}