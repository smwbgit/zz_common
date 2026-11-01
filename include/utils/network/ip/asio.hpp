#pragma once

#include <utils/network/ip.hpp>
#include <boost/asio.hpp>

// 把基本类型，转为，asio库的表示
namespace utils::network::ip::asio
{
	// 对 同名类 的简单包装
	template <version::Version ip_version>
	class Ip
	{
	private:
		utils::network::ip::Ip<ip_version> m_ip;
		boost::asio::ip::address m_ip_asio;

	public:
		// 构造函数
		Ip(const utils::network::ip::Ip<ip_version> & ip)
			: m_ip{ip}
		{
			// 对于 ipv4
			if constexpr (ip_version::version == 4)
			{
				boost::asio::ip::address_v4::bytes_type bytes{};
				std::copy(ip.m_data.begin(), ip.m_data.end(), bytes.begin());
				m_ip_asio = boost::asio::ip::address_v4(bytes);
			}
			// 对于 ipv6
			else if constexpr (ip_version::version == 6)
			{
				boost::asio::ip::address_v6::bytes_type bytes{};
				std::copy(ip.m_data.begin(), ip.m_data.end(), bytes.begin());
				m_ip_asio = boost::asio::ip::address_v6(bytes);
			}
			// 报错
			else
			{
				// 报错
				static_assert(false);
			}
		}
		// 构造函数
		Ip(const boost::asio::ip::address & ip_asio)
			: m_ip_asio{ ip_asio }
		{
			// 对于 ipv4
			if constexpr (ip_version::version == 4)
			{
				// 简单检查
				utils_assert(ip_asio.is_v4());
				
				// 简单转换
				const auto ip_asio_2 = ip_asio.to_v4();

				// 简单转换
				const auto ip_asio_3 = ip_asio_2.to_bytes();

				// 保存
				m_ip = utils::network::ip::Ip<ip_version>{ ip_asio_3 };
			}
			// 对于 ipv6
			else if constexpr (ip_version::version == 6)
			{
				// 简单检查
				utils_assert(ip_asio.is_v6());
				
				// 简单转换
				const auto ip_asio_2 = ip_asio.to_v6();

				// 简单转换
				const auto ip_asio_3 = ip_asio_2.to_bytes();

				// 保存
				m_ip = utils::network::ip::Ip<ip_version>{ ip_asio_3 };
			}
			// 报错
			else
			{
				// 报错
				static_assert(false);
			}
		}

	public:
		// 获取 ip
		const utils::network::ip::Ip<ip_version> & get() const
		{
			return m_ip;
		}
		// 特化
		const boost::asio::ip::address & get_asio() const
		{
			return m_ip_asio;
		}
		// 特化
		operator const utils::network::ip::Ip<ip_version> &() const
		{
			return get();
		}
		// 特化
		operator const boost::asio::ip::address &() const
		{
			return get_asio();
		}
	};

	namespace inner
	{
		// 对 同名类 的简单包装
		template <version::Version ip_version, typename T_endpoint>
		class Endpoint
		{
		private:
			utils::network::ip::Endpoint<ip_version> m_endpoint;
			T_endpoint m_endpoint_asio;

		public:
			// 构造函数
			Endpoint(const utils::network::ip::Endpoint<ip_version> & endpoint)
				: m_endpoint{ endpoint }
				, m_endpoint_asio{ utils::network::ip::asio::Ip<ip_version>{ endpoint.ip }.get_asio(), endpoint.port }
			{
			}
			// 特化
			Endpoint(const T_endpoint & endpoint_asio)
				: m_endpoint{ utils::network::ip::asio::Ip<ip_version>{ endpoint_asio.address() }.get(), endpoint_asio.port()}
				, m_endpoint_asio{ endpoint_asio }
			{
			}

		public:
			// 获取
			const utils::network::ip::Endpoint<ip_version> & get() const
			{
				return m_endpoint;
			}
			// 特化
			const T_endpoint & get_asio() const
			{
				return m_endpoint_asio;
			}
			// 特化
			operator const utils::network::ip::Endpoint<ip_version> & () const
			{
				return get();
			}
			// 特化
			operator const T_endpoint & () const
			{
				return get_asio();
			}
		};
	}

	template <version::Version ip_version>
	using TcpEndpoint = inner::Endpoint<ip_version, boost::asio::ip::tcp::endpoint>;

	template <version::Version ip_version>
	using UdpEndpoint = inner::Endpoint<ip_version, boost::asio::ip::udp::endpoint>;
};