#pragma once

#include <string_view>
#include <utils/assert.hpp>
#include <utils/bit.hpp>
#include <utils/text/namespace.hpp>
#include <utils/string.hpp>

namespace utils::network::tcp
{
	//网段
	class NetworkSegment
	{
	private:
		uint8_t m_first_ip[4]{};
		uint64_t m_size{};
		uint64_t m_bit_count{};
	public:
		void reset(std::string_view network_segment)
		{
			auto pos = network_segment.find('/');
			utils_assert(pos!= network_segment.npos);

			m_bit_count = (uint64_t)(utils::string)network_segment.substr(pos+1);
			m_size = pow(2, m_bit_count);

			auto ip = (utils::string)network_segment.substr(0, pos);

			auto frags = ip.string_view().split(".");
			utils_assert(frags.size() == 4);
			for (int i = 0; i < 4; i++)
				m_first_ip[i] = (uint8_t)(utils::string)frags[i];
		}
		//is_from_high: 是否从高位开始
		void get_addr(uint64_t ip_index, std::string & ip, bool is_from_high)
		{
			utils_assert(ip_index < m_size);
		
			//从高位开始
			if (is_from_high)
			{
				ip_index = utils::bit::reverse_low_bits(ip_index, m_bit_count);
				utils_assert(ip_index < m_size);
			}

			uint8_t _ip[4]{};
			auto _ip_index = ip_index;
			for (int i = 3; i >= 0; i--)
			{
				uint64_t number = _ip_index % 256 + m_first_ip[i];
				utils_assert(number < 256);

				_ip[i] = (uint8_t)number;
				_ip_index /= 256;
			}

			ip = utils::text::format("%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
		}
		uint64_t get_size()
		{
			return m_size;
		}
	};
}
