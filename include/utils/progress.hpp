#pragma once

#include <vector>
#include "utils/assert.hpp"
#include "utils/iterator.hpp"

namespace utils::progress
{
	//进度合成器
	//note: 把整个进度划分为指定的若干部分，当给出所处部分和所处部分的进度时，返回整体进度
	//e.g.:
	//		划分为0.3和0.7
	//		get(0, 0.5)   ->   0.3 * 0.5   =  0.15
	//		get(1, 0.5)   ->   0.3 + 0.7 * 0.5   =  0.65
	class Synthesizer
	{
	private:
		std::vector<float> m_parts;
	public:
		Synthesizer(std::vector<float> parts)
			: m_parts{parts}
		{
			utils_assert(parts.empty() == false);
		}

		//获得进度
		//part_index：所处部分
		//progress_in_this_part：该部分中的进度
		void get(_Out_ float & progress, _In_ uint64_t part_index, _In_ float progress_in_this_part)
		{
			//简单检查
			utils_assert(part_index < m_parts.size());

			//计算
			progress = 0;
			for (const auto i : utils::iterator_index(part_index))
			{
				progress += m_parts[i];
			}
			progress += m_parts[part_index] * progress_in_this_part;

			//更正，如果需要
			if (progress < 0)
				progress = 0;
			if (progress > 1)
				progress = 1;
		}
	};

	using Receiver = std::function<void(float p)>;

	//进度报告器
	//note: 可以把整个进度划分为指定的若干部分，自动向上级报告器报告
	class Reporter
	{
	private:
		Receiver m_receiver;
		Synthesizer m_synthesizer;

	public:
		//receiver: 上级报告器
		Reporter(Receiver receiver, std::vector<float> parts)
			: m_receiver{receiver}
			, m_synthesizer{parts}
		{
		}
		
		//获得指定部分的报告器
		//part_index：所处部分
		Receiver get_receiver(_In_ uint64_t part_index)
		{
			return [=](float p)
			{
				float _p{};
				m_synthesizer.get(_p, part_index, p);
				m_receiver(_p);
			};
		}
	};
}