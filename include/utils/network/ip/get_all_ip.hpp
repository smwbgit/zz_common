#pragma once

#include <utils/network/ip/asio.hpp>

namespace utils::network::ip
{
	// 获取本机ip
	// note: 不包括 回环和通配地址
	template <utils::network::ip::Version ip_version>
	std::vector<utils::network::ip::Ip<ip_version>> get_all_ip()
	{
		// ai生成后整理

		// 准备数据
		boost::asio::io_context context;
		boost::asio::ip::tcp::resolver resolver{ context };
    
		// 获取本机所有网卡上的 ip 地址
		const auto all_ip = resolver.resolve(boost::asio::ip::host_name(), "");
    
		// 准备数据
		std::vector<utils::network::ip::Ip<ip_version>> all_ip_2;

		// 对于每个 ip
		for (const auto & ip : all_ip)
		{
			// 简单转换
			const auto ip_2 = ip.endpoint().address();

			// 跳过回环和通配地址
			if (ip_2.is_loopback() || ip_2.is_unspecified())
				continue;

			// 简单检查
			utils_assert(ip_version::version == 4 || ip_version::version == 6);

			// 版本相同
			if (ip_version::version == 4 && ip_2.is_v4() ||
				ip_version::version == 6 && ip_2.is_v6() )
			{
				// 简单转换
				const utils::network::ip::asio::Ip<ip_version> ip_3{ ip_2 };

				// 简单转换
				const auto ip_4 = ip_3.get();

				// 保存
				all_ip_2.push_back(ip_4);
			}
		}

		//给出
		return all_ip_2;
	}
}