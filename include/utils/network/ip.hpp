#pragma once

#include <array>
#include <utils/text/hex.hpp>
#include <utils/text/format.hpp>
#include <utils/data.hpp>
#include <utils/basic/change_byte_order.hpp>
#include <utils/SizedData.hpp>

// inet_pton 需要：
#ifdef utils_on_windows
#	include <ws2tcpip.h>
#elif defined(utils_on_linux) || defined(utils_on_android)
#	include <arpa/inet.h>
#else
#	error
#endif

//ip协议版本
namespace utils::network::ip::version
{
	struct v4
	{
	public:
		static inline const u8 version = 4;
		static inline const u8 ip_size = 4;
		static inline const std::u8string text_short = u8"v4";
		static inline const u8 ip_provider_prefix_size = 4;
	};
	struct v6
	{
	public:
		static inline const u8 version = 6;
		static inline const u8 ip_size = 16;
		static inline const std::u8string text_short = u8"v6";
		static inline const u8 ip_provider_prefix_size = 8;
	};

	//ip协议版本
	template<typename T>
	concept Version = requires
	{
		{ T::version } -> std::same_as<const u8 &>;
		{ T::ip_size } -> std::same_as<const u8 &>;
		{ T::text_short } -> std::same_as<const std::u8string &>;
		{ T::ip_provider_prefix_size } -> std::same_as<const u8 &>;
	};

	//简单检查
	static_assert(Version<v4>);
	static_assert(Version<v6>);
}
namespace utils::network::ip
{
	//简化名字空间
	using utils::network::ip::version::Version;
}
namespace utils::network::ip
{
	//代表一个ip地址中，网络服务商分配的、公网路由认的、用户改不了的那部分
	//note: 当ipv4时，使用全部4字节。当ipv6时，使用前8字节
	//note: 比如，可以用于对ip的去重
	template<version::Version ip_version>
	class IpProviderPrefix
	{
	public:
		utils::SizedData<ip_version::ip_provider_prefix_size> m_data;
	public:
		//比较运算
		auto operator<=>(const IpProviderPrefix &) const = default;
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
	};

	//代表一个ip地址
	template<version::Version ip_version>
	class Ip
	{
	public:
		using IpProviderPrefix = IpProviderPrefix<ip_version>;
	public:
		utils::SizedData<ip_version::ip_size> m_data;
	public:
		//构造函数
		explicit Ip(utils::data_view data)
			: m_data{data}
		{
		}
		//构造函数
		explicit Ip(const std::u8string & ip)
		{
			//简单转换
			const auto af = []() -> int
			{
				if constexpr (ip_version::version == 4)
					return AF_INET;
				else if constexpr (ip_version::version == 6)
					return AF_INET6;
				else
				{
					//报错
					static_assert(false);
				}
			}();

			//进行转换
			utils_assert(::inet_pton(af, (const char*)ip.c_str(), m_data.addr()) == 1);
		}
		//构造函数
		Ip() = default;
		Ip(const Ip &) = default;
		Ip(Ip &&) = default;
	public:
		//赋值函数
		Ip & operator = (utils::data_view data)
		{
			m_data = data;
			return *this;
		}
		//赋值函数
		Ip & operator = (const Ip &) = default;
		Ip & operator = (Ip &&) = default;
	public:
		//把ip地址，转为"x.x.x.x"的文本格式（对于ipv4），或者"x:x:x:x:x:x"的文本格式（对于ipv6）
		std::u8string to_text_human() const
		{
			if constexpr(ip_version::version == 4)
			{
				//拼出文本
				const auto text = utils::text::format::format(u8"%d.%d.%d.%d",m_data[0],m_data[1],m_data[2],m_data[3]);

				//给出
				return text;
			}
			else if constexpr(ip_version::version == 6)
			{
				//拼出文本
				const auto text = utils::text::format::format(
					u8"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
					m_data[0], m_data[1], m_data[2], m_data[3],
					m_data[4], m_data[5], m_data[6], m_data[7],
					m_data[8], m_data[9], m_data[10],m_data[11],
					m_data[12],m_data[13],m_data[14],m_data[15]);

				//给出
				return text;
			}
			else
			{
				//报错
				static_assert(false);
			}
		}
		//获取服务商前缀
		IpProviderPrefix get_provider_prefix() const
		{
			//准备数据
			IpProviderPrefix prefix;

			//复制
			prefix.m_data = m_data.sub_data(0, prefix.m_data.size());

			//给出
			return prefix;
		}
		//比较运算
		auto operator<=>(const Ip<ip_version> &) const = default;
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
	};

	//代表一个2字节的端口号
	class Port
	{
	public:
		u2 m_data{};
	public:
		Port(u2 port = 0)
			: m_data{port}
		{
		}
	public:
		//大小端转换
		void change_byte_order()
		{
			//进行转换
			utils::basic::change_byte_order(m_data);
		}
		//隐式转换
		operator u2() const
		{
			return m_data;
		}
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
	};

	//代表一个通信端点
	template<version::Version ip_version>
	class Endpoint
	{
	public:
		Ip<ip_version> ip;
		Port port;

	public:
		Endpoint() = default;
		Endpoint(const Ip<ip_version> & ip, const Port & port)
			: ip{ ip }
			, port{ port }
		{

		}
		Endpoint(const std::u8string & ip, const u2 port)
			: ip{ ip }
			, port{ port }
		{

		}
		Endpoint(utils::data_view ip, const u2 port)
			: ip{ ip }
			, port{ port }
		{

		}
		Endpoint(const Endpoint<ip_version> &) = default;
		Endpoint(Endpoint<ip_version> &&) = default;

	public:
		Endpoint<ip_version> & operator = (const Endpoint<ip_version> &) = default;
		Endpoint<ip_version> & operator = (Endpoint<ip_version> &&) = default;

	public:
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & ip;
			file & port;
		}
	};
};